/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef NANCY_STATE_SCENE_H
#define NANCY_STATE_SCENE_H

#include "engines/nancy/action/actionmanager.h"

#include "engines/nancy/ui/frame.h"
#include "engines/nancy/ui/viewport.h"
#include "engines/nancy/ui/textbox.h"
#include "engines/nancy/ui/inventorybox.h"

#include "engines/nancy/time.h"
#include "engines/nancy/commontypes.h"
#include "engines/nancy/nancy.h"
#include "engines/nancy/sound.h"

#include "common/scummsys.h"
#include "common/array.h"
#include "common/str.h"

namespace Graphics {
	struct Surface;
}

namespace Common {
    class SeekableReadStream;
}

namespace Nancy {

class NancyEngine;

namespace State {

struct SceneInfo {
    uint16 sceneID = 0;
    uint16 frameID = 0;
    uint16 verticalOffset = 0;
};

class Scene {
    friend class Nancy::Action::ActionRecord;
    friend class Nancy::Action::ActionManager;
    friend class Nancy::NancyConsole;
    friend class Nancy::NancyEngine;
public:
    struct SceneSummary { // SSUM
        Common::String description;             // 0x00
        Common::String videoFile;               // 0x32
        //
        uint16 videoFormat;                     // 0x3E, value is 1 or 2
        Common::String audioFile;               
        SoundDescription sound;   // 0x40
        //
        uint16 verticalScrollDelta;             // 0x72
        uint16 horizontalEdgeSize;              // 0x74
        uint16 verticalEdgeSize;                // 0x76
        Time slowMoveTimeDelta;                 // 0x78
        Time fastMoveTimeDelta;                 // 0x7A
        // byte unknown7C enum with 4 values
        //
    };

    Scene(Nancy::NancyEngine *engine) :
        _engine (engine),
        _state (kInit),
        _frame(engine),
        _lastHint(-1),
        _gameStateRequested(NancyEngine::kScene),
        _viewport(engine),
        _textbox(_frame),
        _inventoryBox(_frame),
        _actionManager(engine) {}

    void process();

    void changeScene(uint16 id, uint16 frame, uint16 verticalOffset, bool noSound);
    void changeScene(const SceneChangeDescription &sceneDescription);
    void pushScene();
    void popScene();

    void pauseSceneSpecificSounds();
    void unpauseSceneSpecificSounds();

    void addItemToInventory(uint16 id);
    void removeItemFromInventory(uint16 id, bool pickUp = true);
    int16 getHeldItem() const { return _flags.heldItem; }
    void setHeldItem(int16 id) { _flags.heldItem = id; _engine->cursorManager->setCursorItemID(id); }
    NancyFlag hasItem(int16 id) const { return _flags.items[id]; }

    void setEventFlag(int16 label, NancyFlag flag = kTrue);
    void setEventFlag(EventFlagDescription eventFlag);
    bool getEventFlag(int16 label, NancyFlag flag = kTrue) const;
    bool getEventFlag(EventFlagDescription eventFlag) const;

    void setLogicCondition(int16 label, NancyFlag flag = kTrue);
    bool getLogicCondition(int16 label, NancyFlag flag = kTrue) const;
    void clearLogicConditions();

    void setDifficulty(uint difficulty) { _difficulty = difficulty; }
    uint16 getDifficulty() const { return _difficulty; }

    byte getHintsRemaining() const { return _hintsRemaining[_difficulty]; }
    void useHint(int hintID, int hintWeight);

    void requestStateChange(NancyEngine::GameState state) { _gameStateRequested = state; }

    void resetAndStartTimer() { _timers.timerIsActive = true; _timers.timerTime = 0; }
    void stopTimer() { _timers.timerIsActive = false; _timers.timerTime = 0; }

    Time getMovementTimeDelta(bool fast) const { return fast ? _sceneState.summary.fastMoveTimeDelta : _sceneState.summary.slowMoveTimeDelta; }

    void registerGraphics();

    UI::Frame &getFrame() { return _frame; }
    UI::Viewport &getViewport() { return _viewport; }
    UI::Textbox &getTextbox() { return _textbox; }
    UI::InventoryBox &getInventoryBox() { return _inventoryBox; }

    Action::ActionManager &getActionManager() { return _actionManager; }

    SceneInfo &getSceneInfo() { return _sceneState.currentScene; }
    const SceneSummary &getSceneSummary() const { return _sceneState.summary; }

private:
    void init();
    void load();
    void run();

    void readSceneSummary(Common::SeekableReadStream &stream);

    bool changeGameState(bool keepGraphics = false);

    void clearSceneData();

public:
    enum State {
        kInit,
        kLoad,
        kStartSound,
        kRun,
        kLoadNew
    };

    enum GameStateChange : byte {
        kHelpMenu = 1 << 0,
        kMainMenu = 1 << 1,
        kSaveLoad = 1 << 2,
        kReloadSave = 1 << 3,
        kSetupMenu = 1 << 4,
        kCredits = 1 << 5,
        kMap = 1 << 6
    };
    
    // TODO move 
    Time playerTimeMinuteLength;

protected:
    struct SceneState {
        SceneSummary summary;
        SceneInfo currentScene;
        SceneInfo nextScene;
        SceneInfo pushedScene;
        bool isScenePushed;
        byte sceneHitCount[1000];

        bool _doNotStartSound = false;
    };

    struct Timers {
        enum TimeOfDay { kDay = 0, kNight = 1, kDuskDawn = 2 };

        Time tickCount;
        Time pushedPlayTime;

        Time totalTime;
        Time sceneTime;
        Time timerTime;
        bool timerIsActive = false;
        Time playerTime; // In-game time of day, adds a minute every 5 seconds
        Time playerTimeNextMinute; // Stores the next tick count until we add a minute to playerTime
        TimeOfDay timeOfDay = kDay;
    };

    struct PlayFlags {
        struct LogicCondition {
            NancyFlag flag = NancyFlag::kFalse;
            Time timestamp;
        };

        LogicCondition logicConditions[30];
        NancyFlag eventFlags[168];
        NancyFlag items[11];
        int16 heldItem = -1;
        int16 primaryVideoResponsePicked = -1;
    };

    Nancy::NancyEngine *_engine;

    // RenderObjects
    UI::Frame _frame;
    UI::Viewport _viewport;
    UI::Textbox _textbox;
    UI::InventoryBox _inventoryBox;

    // Data
    SceneState _sceneState;
    PlayFlags _flags;
    Timers _timers;
    uint16 _difficulty;
    Common::Array<byte> _hintsRemaining;
    int16 _lastHint;
    NancyEngine::GameState _gameStateRequested;

    Action::ActionManager _actionManager;

    State _state;

    bool isComingFromMenu = true;
    bool hasLoadedFromSavefile = false;
};

} // End of namespace State
} // End of namespace Nancy

#endif // NANCY_STATE_SCENE_H