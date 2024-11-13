# Cap'n Proto schema
@0xa7829f89062090f7;

struct KeyPressType {
  # this enum should equal the KeyType enum in keys.h so we can use the int value
  enum Key {
    forward @0;
    backward @1;
    left @2;
    right @3;
    jump @4;
    aux1 @5;
    sneak @6;
    autoforward @7;
    dig @8;
    place @9;
    esc @10;
    drop @11;
    # Currently, using the inventory key will open the inventory, but pressing it again
    # will not close it. There another key handling pipeline for forms / formspec that we'd
    # need to deal with. The minetest game inventory is implemented in minetest_game/inventory.lua.
    # keyHandling is probably in guiFormspecMenu.cpp.
    inventory @12;
    chat @13;
    cmd @14;
    cmdLocal @15;
    console @16;
    minimap @17;
    freemove @18;
    pitchmove @19;
    fastmove @20;
    noclip @21;
    hotbarPrev @22;
    hotbarNext @23;
    mute @24;
    incVolume @25;
    decVolume @26;
    cinematic @27;
    screenshot @28;
    toggleBlockBounds @29;
    toggleHud @30;
    toggleChat @31;
    toggleFog @32;
    toggleUpdateCamera @33;
    toggleDebug @34;
    toggleProfiler @35;
    cameraMode @36;
    increaseViewingRange @37;
    decreaseViewingRange @38;
    rangeselect @39;
    zoom @40;
    quicktuneNext @41;
    quicktunePrev @42;
    quicktuneInc @43;
    quicktuneDec @44;
    slot1 @45;
    slot2 @46;
    slot3 @47;
    slot4 @48;
    slot5 @49;
    slot6 @50;
    slot7 @51;
    slot8 @52;
    slot9 @53;
    slot10 @54;
    slot11 @55;
    slot12 @56;
    slot13 @57;
    slot14 @58;
    slot15 @59;
    slot16 @60;
    slot17 @61;
    slot18 @62;
    slot19 @63;
    slot20 @64;
    slot21 @65;
    slot22 @66;
    slot23 @67;
    slot24 @68;
    slot25 @69;
    slot26 @70;
    slot27 @71;
    slot28 @72;
    slot29 @73;
    slot30 @74;
    slot31 @75;
    slot32 @76;
    # here we can add extra keys
    # note: add them to inputhandler.h aswell
    middle @77;
    ctrl @78;
    # used for iteration
    internalEnumCount @79;
  }
}

struct Action {
  keyEvents @0 :List(KeyPressType.Key);
  mouseDx @1 :Int32;
  mouseDy @2 :Int32;
}

struct Image {
  width @0 :Int32;
  height @1 :Int32;
  data @2 :Data;
}

struct AuxMap {
  entries @0 :List(Entry);
  struct Entry {
    key @0 :Text;
    value @1 :Float32;
  }
}

struct Observation {
  image @0 :Image;
  # TODO: make this optional so that the agent can tell if the game is done loading.
  reward @1 :Float32;
  done @2 :Bool;
  aux @3 :AuxMap;
}

interface Minetest {
  init @0 () -> ();
  step @1 (action :Action) -> (observation :Observation);
}
