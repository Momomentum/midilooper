#include "uris.h"

#include <cassert>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/atom/atom.h>
#include <lv2/urid/urid.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <cstdint>

static const char *midilooper_uri = "http://momomentum.de/midilooper";

enum class ports {
    MIDI_IN = 0,
    CONTROL = 1,
    MIDI_OUT = 2
};

struct midilooper {
    LV2_URID_Map *urid_map;
    LV2_Log_Logger logger;

    midilooper_uris uris;

    struct {
        const LV2_Atom_Sequence *in;
        const LV2_Atom_Sequence *control;
        LV2_Atom_Sequence *out;
    } ports;


    double rate{0};  // Sample rate
    float  bpm{0};   // Beats per minute (tempo)
    float  speed{0}; // Transport speed (usually 0=stop, 1=play)
};


struct MIDINoteEvent {
    LV2_Atom_Event event;
    uint8_t msg[3];
};

static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *bundle_path,
                              const LV2_Feature *const *features) {
    auto ml = new midilooper();
    const char*
        missing = lv2_features_query(
                features,
                LV2_LOG__log,
                &ml->logger.log, false,
                LV2_URID__map, &ml->urid_map,
                true,
                NULL);
    if (missing) {
        lv2_log_error(&ml->logger, "Missing feature <%s>\n", missing);
        delete ml;
    }

    map_uris(ml);

    return static_cast<LV2_Handle>(ml);
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
    }
}

static void activate(LV2_Handle instance) {}


static void update_position(midilooper* ml, const LV2_Atom_Object* obj) {
    const auto &uris = &ml->uris;

    // Received new transport position/speed
    LV2_Atom* beat  = NULL;
    LV2_Atom* bpm   = NULL;
    LV2_Atom* speed = NULL;

    lv2_atom_object_get(obj,
            uris->time_barBeat, &beat,
            uris->time_beatsPerMinute, &bpm,
            uris->time_speed, &speed,
            NULL);
    if (bpm && bpm->type == uris->atom_Float) {
        // Tempo changed, update BPM
        ml->bpm = ((LV2_Atom_Float*)bpm)->body;
    }
    if (speed && speed->type == uris->atom_Float) {
        // Speed changed, e.g. 0 (stop) to 1 (play)
        ml->speed = ((LV2_Atom_Float*)speed)->body;
    }
    if (beat && beat->type == uris->atom_Float) {
        MIDINoteEvent note;
        note.msg[0] = LV2_MIDI_MSG_NOTE_ON;
        note.msg[1] = 64;
        note.msg[2] = 127;
        // Write an empty Sequence header to the output
        lv2_atom_sequence_clear(ml->ports.out);
        ml->ports.out->atom.type = uris->midi_MidiEvent;
        lv2_atom_sequence_append_event(ml->ports.out, ml->ports.out->atom.size, &note.event);
    }
}

static void run(LV2_Handle instance, uint32_t n_samples) {
    auto ml = static_cast<midilooper *>(instance);

    const LV2_Atom_Sequence *in = ml->ports.control;

    for (auto ev = lv2_atom_sequence_begin(&in->body);
            !lv2_atom_sequence_is_end(&in->body, in->atom.size, ev);
            ev = lv2_atom_sequence_next(ev)) {

        // Check if this event is an Object
        // (or deprecated Blank to tolerate old hosts)
        if (ev->body.type == ml->uris.atom_Object ||
                ev->body.type == ml->uris.atom_Blank) {
            const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == ml->uris.time_Position) {
                update_position(ml, obj);
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
    midilooper_uri, instantiate, connect_port,  activate, run,
    deactivate,     cleanup,     extension_data};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : nullptr;
}

