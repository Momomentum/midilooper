#include "uris.h"

#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/atom/atom.h>
#include <lv2/urid/urid.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/atom/forge.h>
#include <cstdint>

struct state {
    LV2_Atom_Bool rec;
};

enum class ports {
    MIDI_IN = 0,
    CONTROL = 1,
    MIDI_OUT = 2,
    PARAM_IN = 3
};

struct midilooper {
    LV2_URID_Map *urid_map;
    LV2_URID_Unmap *urid_unmap;
    midilooper_uris uris;

    char *urid_buf[12];

    LV2_Log_Log *log;
    LV2_Log_Logger logger;


    bool note_on{false};

    LV2_Atom_Forge forge;
    LV2_Atom_Forge_Frame out_frame;

    struct {
        const LV2_Atom_Sequence *in;
        const LV2_Atom_Sequence *control;
        LV2_Atom_Sequence *out;
        const LV2_Atom_Sequence *param_in;
    } ports;


    double rate{0};  // Sample rate
    float  bpm{0};   // Beats per minute (tempo)
    float  speed{0}; // Transport speed (usually 0=stop, 1=play)
    long bar{0};

    bool rec{false};
};

static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *bundle_path,
                              const LV2_Feature *const *features) {
    auto ml = new midilooper();
    const char*
        missing = lv2_features_query(
                features,
                LV2_LOG__log, &ml->log, false,
                LV2_URID__map, &ml->urid_map, true,
                LV2_URID__unmap, &ml->urid_unmap, false,
                NULL);

    lv2_log_logger_init(&ml->logger, ml->urid_map, ml->log);

    if (missing) {
        lv2_log_error(&ml->logger, "Missing feature <%s>\n", missing);
        delete ml;
    }

    map_uris(ml);

    lv2_atom_forge_init(&ml->forge, ml->urid_map);

    return ml;
}

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    auto ml = static_cast<midilooper*>(instance); 
    switch (static_cast<ports>(port)) {
        case ports::MIDI_IN:
           ml->ports.in = static_cast<const LV2_Atom_Sequence *>(data);
           break; 
        case ports::CONTROL:
           ml->ports.control = static_cast<const LV2_Atom_Sequence *>(data);
           break; 
        case ports::MIDI_OUT:
           ml->ports.out = static_cast<LV2_Atom_Sequence *>(data);
           break;
        case ports::PARAM_IN:
           ml->ports.param_in = static_cast<const LV2_Atom_Sequence *>(data);
           break;
        default:
           return;
    }
}

static void activate(LV2_Handle instance) {}

static void forge_midi_event(midilooper *ml, uint32_t ts, const uint8_t* const buffer, uint32_t size) {
    auto &uris = ml->uris;
    auto &forge = ml->forge;

    LV2_Atom midi_atom;
    midi_atom.type = uris.midi_MidiEvent;
    midi_atom.size = size;

    if (0 == lv2_atom_forge_frame_time (&forge, ts)) return;
    if (0 == lv2_atom_forge_raw (&forge, &midi_atom, sizeof (LV2_Atom))) return;
    if (0 == lv2_atom_forge_raw (&forge, buffer, size)) return;
    lv2_atom_forge_pad (&forge, sizeof (LV2_Atom) + size);
}

static void update_position(midilooper* ml, const LV2_Atom_Object* obj, const LV2_Atom_Event *ev) {
    const auto &uris = &ml->uris;

    // Received new transport position/speed
    LV2_Atom* bar  = nullptr;
    LV2_Atom* beat  = nullptr;
    LV2_Atom* beatUnit  = nullptr;
    LV2_Atom* bpm   = nullptr;
    LV2_Atom* speed = nullptr;

    lv2_atom_object_get(obj,
            uris->time_bar, &bar,
            uris->time_barBeat, &beat,
            uris->time_beatUnit, &beatUnit,
            uris->time_beatsPerMinute, &bpm,
            uris->time_speed, &speed,
            NULL);
    if (bpm && bpm->type == uris->atom_Float &&
        speed && speed->type == uris->atom_Float &&
        bar && bar->type == uris->atom_Long &&
        beat && beat->type == uris->atom_Float &&
        beatUnit && beatUnit->type == uris->atom_Int) {

        ml->bpm = reinterpret_cast<LV2_Atom_Float*>(bpm)->body;
        ml->speed = reinterpret_cast<LV2_Atom_Float*>(speed)->body;
        auto _bar = reinterpret_cast<LV2_Atom_Long*>(bar)->body;

        // TODO
        auto beat_unit = reinterpret_cast<LV2_Atom_Int*>(beatUnit)->body;
        auto _beat = reinterpret_cast<LV2_Atom_Float*>(beat)->body;

        if (ml->bar != _bar) {
            ml->bar = _bar;
            lv2_log_note(&ml->logger, "Trigger Midi\n");

            ml->note_on = !ml->note_on;

            LV2_Atom_Event note_event;

            // TEST
            // uint8_t chan = 0;
            // uint8_t d[3];
            // d[0] = (ml->note_on ? LV2_MIDI_MSG_NOTE_ON : LV2_MIDI_MSG_NOTE_OFF) | chan;
            // d[1] = 60;
            // d[2] = 127;
            // forge_midi_event(ml, ev->time.frames, d, 3);
        }
    }
}

static void run(LV2_Handle instance, uint32_t n_samples) {
    auto ml = static_cast<midilooper *>(instance);
    {
        const auto in = ml->ports.control;

        const uint32_t capacity = ml->ports.out->atom.size;
        lv2_atom_forge_set_buffer(&ml->forge, (uint8_t*)ml->ports.out, capacity);
        lv2_atom_forge_sequence_head(&ml->forge, &ml->out_frame, 0);

        for (auto ev = lv2_atom_sequence_begin(&in->body);
                !lv2_atom_sequence_is_end(&in->body, in->atom.size, ev);
                ev = lv2_atom_sequence_next(ev)) {

            // Check if this event is an Object
            // (or deprecated Blank to tolerate old hosts)
            if (ev->body.type == ml->uris.atom_Object ||
                    ev->body.type == ml->uris.atom_Blank) {
                auto obj = reinterpret_cast<const LV2_Atom_Object*>(&ev->body);
                if (obj->body.otype == ml->uris.time_Position) {
                    // update_position(ml, obj, ev);
                }
            }
        }

        // Close the sequence
        lv2_atom_forge_pop(&ml->forge, &ml->out_frame);
    }
    {
        const auto in = ml->ports.param_in;

        for (auto ev = lv2_atom_sequence_begin(&in->body);
                !lv2_atom_sequence_is_end(&in->body, in->atom.size, ev);
                ev = lv2_atom_sequence_next(ev)) {
            lv2_log_note(&ml->logger, "Patch Set\n");
            auto obj = reinterpret_cast<const LV2_Atom_Object*>(&ev->body);
            if (obj->body.otype == ml->uris.patch_Set) {
                lv2_log_note(&ml->logger, "Patch Set\n");
            }
        }
    }
}

static void deactivate(LV2_Handle instance) {}

static void cleanup(LV2_Handle instance) {
    delete static_cast<midilooper *>(instance);
}

static const void *extension_data(const char *uri) { return nullptr; }

static const LV2_Descriptor descriptor = {
    ml::uri.data(), instantiate, connect_port,  activate, run,
    deactivate,     cleanup,     extension_data};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : nullptr;
}

