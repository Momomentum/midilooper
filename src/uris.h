#pragma once

#include <lv2/atom/atom.h>
#include <lv2/midi/midi.h>
#include <lv2/time/time.h>
#include <lv2/patch/patch.h>
#include <lv2/urid/urid.h>

struct midilooper_uris {
    LV2_URID atom_Blank;
    LV2_URID atom_Float;
    LV2_URID atom_Object;
    LV2_URID atom_Path;
    LV2_URID atom_Resource;
    LV2_URID atom_Sequence;
    LV2_URID atom_URID;
    LV2_URID atom_eventTransfer;
    LV2_URID atom_Event;
    LV2_URID time_Position;
    LV2_URID time_barBeat;
    LV2_URID time_beatsPerMinute;
    LV2_URID time_speed;
    LV2_URID midi_MidiEvent;
    LV2_URID patch_Set;
    LV2_URID patch_property;
    LV2_URID patch_value;
};

template<typename T> void map_uris(T *ml) {
    auto &uris = ml->uris;
    LV2_URID_Map *map = ml->urid_map;

    uris.atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
    uris.atom_Float = map->map(map->handle, LV2_ATOM__Float);
    uris.atom_Object = map->map(map->handle, LV2_ATOM__Object);
    uris.atom_Path = map->map(map->handle, LV2_ATOM__Path);
    uris.atom_Resource = map->map(map->handle, LV2_ATOM__Resource);
    uris.atom_Sequence = map->map(map->handle, LV2_ATOM__Sequence);
    uris.atom_URID = map->map(map->handle, LV2_ATOM__URID);
    uris.atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    uris.atom_Event = map->map(map->handle, LV2_ATOM__Event);
    uris.time_Position = map->map(map->handle, LV2_TIME__position);
    uris.time_barBeat = map->map(map->handle, LV2_TIME__barBeat);
    uris.time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
    uris.time_speed = map->map(map->handle, LV2_TIME__speed);
    uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
    uris.patch_Set = map->map(map->handle, LV2_PATCH__Set);
    uris.patch_property = map->map(map->handle, LV2_PATCH__property);
    uris.patch_value = map->map(map->handle, LV2_PATCH__value);
};
