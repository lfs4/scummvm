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
#include "common/config-manager.h"
#include "engines/util.h"
#include "graphics/thumbnail.h"
#include "graphics/surface.h"
#include "common/error.h"
#include "actor.h"
#include "actorresource.h"
#include "background.h"
#include "bigfile.h"
#include "cursor.h"
#include "dragonflg.h"
#include "dragonimg.h"
#include "dragonini.h"
#include "dragonobd.h"
#include "dragonrms.h"
#include "dragonvar.h"
#include "dragons.h"
#include "inventory.h"
#include "scene.h"
#include "screen.h"
#include "sequenceopcodes.h"
#include "scriptopcodes.h"
#include "bag.h"

namespace Dragons {

#define DRAGONS_TICK_INTERVAL 17

static DragonsEngine *_engine = nullptr;

DragonsEngine *getEngine() {
	return _engine;
}

DragonsEngine::DragonsEngine(OSystem *syst) : Engine(syst) {
	_bigfileArchive = NULL;
	_dragonRMS = NULL;
	_backgroundResourceLoader = NULL;
	_screen = NULL;
	_sequenceOpcodes = new SequenceOpcodes(this);
	_scriptOpcodes = NULL;
	_engine = this;
	_inventory = new Inventory(this);
	_cursor = new Cursor(this);

	_leftMouseButtonUp = false;
	_rightMouseButtonUp = false;
	_iKeyUp = false;
	reset();
}

DragonsEngine::~DragonsEngine() {
	delete _sequenceOpcodes;
	delete _scriptOpcodes;
}

void DragonsEngine::updateEvents() {
	Common::Event event;
	_leftMouseButtonUp = false;
	_rightMouseButtonUp = false;
	_iKeyUp = false;
	while (_eventMan->pollEvent(event)) {
//		_input->processEvent(event);
		switch (event.type) {
			case Common::EVENT_QUIT:
				quitGame();
				break;
			case Common::EVENT_MOUSEMOVE:
				_cursor->updatePosition(event.mouse.x, event.mouse.y);
				break;
			case Common::EVENT_LBUTTONUP:
				_leftMouseButtonUp = true;
				break;
			case Common::EVENT_RBUTTONUP:
				_rightMouseButtonUp = true;
				break;
			case Common::EVENT_KEYUP:
				if (event.kbd.keycode == Common::KeyCode::KEYCODE_i) {
					_iKeyUp = true;
				}
			default:
				break;
		}
	}
}

Common::Error DragonsEngine::run() {
	_screen = new Screen();
	_bigfileArchive = new BigfileArchive("bigfile.dat", Common::Language::EN_ANY);
	_dragonFLG = new DragonFLG(_bigfileArchive);
	_dragonIMG = new DragonIMG(_bigfileArchive);
	_dragonOBD = new DragonOBD(_bigfileArchive);
	_dragonRMS = new DragonRMS(_bigfileArchive, _dragonOBD);
	_dragonVAR = new DragonVAR(_bigfileArchive);
	_dragonINIResource = new DragonINIResource(_bigfileArchive);
	ActorResourceLoader *actorResourceLoader = new ActorResourceLoader(_bigfileArchive);
	_actorManager = new ActorManager(actorResourceLoader);
	_scriptOpcodes = new ScriptOpcodes(this, _dragonFLG);
	_backgroundResourceLoader = new BackgroundResourceLoader(_bigfileArchive, _dragonRMS);
	_scene = new Scene(this, _screen, _scriptOpcodes, _bigfileArchive, _actorManager, _dragonRMS, _dragonINIResource, _backgroundResourceLoader);

	if (ConfMan.hasKey("save_slot")) {
		loadGameState(ConfMan.getInt("save_slot"));
	} else {
		loadScene(0);
	}

	_scene->draw();
	_screen->updateScreen();

	gameLoop();

	delete _scene;
	delete _actorManager;
	delete _backgroundResourceLoader;
	delete _dragonFLG;
	delete _dragonIMG;
	delete _dragonRMS;
	delete _dragonVAR;
	delete _bigfileArchive;
	delete _screen;

	debug("Ok");
	return Common::kNoError;
}

uint16 DragonsEngine::ipt_img_file_related()
{
	DragonINI *ini;
    DragonINI *flicker = _dragonINIResource->getFlickerRecord();
	assert(flicker);

	int16 tileX = flicker->actor->x_pos / 32;
	int16 tileY = flicker->actor->y_pos / 8;

	for(int i=0;i < _dragonINIResource->totalRecords(); i++) {
		ini = getINI(i);
		if ((ini->sceneId == getCurrentSceneId()) && (ini->field_1a_flags_maybe == 0)) {
			IMG *img = _dragonIMG->getIMG(ini->field_2);
			if ((img->x <= tileX) && (((tileX <= img->x + img->w && (img->y <= tileY)) && (tileY <= img->y + img->h)))) {
				return i + 1;
			}
		}
	}
	return 0;
}

void DragonsEngine::gameLoop()
{
	bool bVar1;
	uint uVar3;
	uint32_t uVar4;
	uint actorId;
	int iVar5;
	ushort uVar6;
	ushort uVar7;
	uint actorId_00;
	void *buffer;
	uint16_t sequenceId;
	DragonINI *pDVar8;
	ushort *puVar9;
	ScriptOpCall local_30;

	_cursor->_cursorActivationSeqOffset = 0;
	bit_flags_8006fbd8 = 0;
	_counter = 0;
	setFlags(ENGINE_FLAG_8);
	actorId = 0;

	while(!shouldQuit()) {
		_scene->draw();
		_screen->updateScreen();
		wait();
		updateHandler();
		updateEvents();

		if (getCurrentSceneId() != 2) {
			_sceneId1 = getCurrentSceneId();
		}

		_counter++;
		if (0x4af < _counter) {
			pDVar8 = _dragonINIResource->getFlickerRecord();
			if (pDVar8->actor->resourceID == 0xe) {
				pDVar8->actor->_sequenceID2 = 2;
				pDVar8->field_20_actor_field_14 = 2;
				if (getINI(0xc2)->field_1e == 1) {
					sequenceId = 0x30;
				}
				else {
					sequenceId = 2;
				}
				pDVar8->actor->updateSequence(sequenceId);
				_counter = 0;
				setFlags(ENGINE_FLAG_80000000);
			}
		}
		if (isFlagSet(ENGINE_FLAG_80000000)
			&& _dragonINIResource->getFlickerRecord()->actor->isFlagSet(ACTOR_FLAG_4)) {
			_counter = 0;
			clearFlags(ENGINE_FLAG_80000000);
		}
		if (bit_flags_8006fbd8 == 0) {
			setFlags(ENGINE_FLAG_8);
		}
		if (_dragonINIResource->getFlickerRecord()->sceneId == getCurrentSceneId()) {
			uVar3 = ipt_img_file_related();
			actorId_00 = uVar3 & 0xffff;
			if (actorId_00 == 0) goto LAB_80026d34;
			if (actorId_00 != (actorId & 0xffff)) {
				//TODO implement this
				error("actorId_00 != (actorId & 0xffff)");
//				buffer = (void *)((int)dragon_Obd_Offset + dragon_Opt_Offset[actorId_00 - 1].obdOffset);
//				local_30.code = (void *)((int)buffer + 8);
//				uVar4 = read_int32(buffer);
//				local_30.codeEnd = (void *)(uVar4 + (int)local_30.code);
//				actorId = run_script_field8_eq_4(&local_30);
//				if ((actorId & 0xffff) != 0) {
//					local_30.codeEnd =
//							(void *)((uint)*(ushort *)((int)local_30.code + 2) +
//									 (int)(void *)((int)local_30.code + 4));
//					local_30.code = (void *)((int)local_30.code + 4);
//					run_script(&local_30);
//					_counter = 0;
//				}
			}
		}
		else {
			LAB_80026d34:
			uVar3 = 0;
		}

		if (_cursor->updateINIUnderCursor() == 0 ||
			(!(_cursor->_iniUnderCursor & 0x8000) && (getINI(_cursor->_iniUnderCursor - 1)->field_1a_flags_maybe & 0x4000) != 0)) { //TODO check this. This logic looks a bit strange.
			_cursor->_cursorActivationSeqOffset = 0;
		}
		else {
			_cursor->_cursorActivationSeqOffset = 5;
		}

		if (_rightMouseButtonUp && isInputEnabled()) {
			_cursor->selectPreviousCursor();
			_counter = 0;
			actorId = uVar3;
			continue;
		}
		// TODO implement cycle cursor up.
//		actorId = CheckButtonMapPress_CycleUp(0);
//		if (((actorId & 0xffff) != 0) && isInputEnabled()) {
//			_cursor->_sequenceID = _cursor->_sequenceID + 1;
//			if (_cursor->iniItemInHand == 0) {
//				bVar1 = _cursor->_sequenceID < 5;
//			}
//			else {
//				bVar1 = _cursor->_sequenceID < 6;
//			}
//			if (!bVar1) {
//				_cursor->_sequenceID = 0;
//			}
//			if ((_cursor->_sequenceID == 0) && ((_inventory->getType() == 1 || (_inventory->getType() == 2)))) {
//				_cursor->_sequenceID = 1;
//			}
//			if (_cursor->_sequenceID == 2) {
//				if (_inventory->getType() == 1) {
//					_cursor->_sequenceID = 4;
//				}
//				uVar6 = 3;
//				if (_cursor->_sequenceID == 2) goto LAB_80026fb0;
//			}
//			_counter = 0;
//			actorId = uVar3;
//			continue;
//		}

		if (bit_flags_8006fbd8 == 3) {
			bit_flags_8006fbd8 = 0;
			DragonINI *flicker = _dragonINIResource->getFlickerRecord();
			if (flicker->sceneId == getCurrentSceneId() && flicker->actor->_sequenceID2 != -1) {
				uVar6 = _scriptOpcodes->_data_800728c0;
				if (_cursor->_sequenceID != 5) {
					uVar6 = _cursor->data_80072890;
				}
				if (uVar6 > 0) {
					flicker->actor->_sequenceID2 = getINI(uVar6 - 1)->field_e;
				}
			}

			works_with_obd_data_1();
			if ((getCurrentSceneId() == 0x1d) && (getINI(0x179)->field_2 != 0))
			{
				clearFlags(ENGINE_FLAG_8);
			} else {
				setFlags(ENGINE_FLAG_8);
			}
			_counter = 0;
			actorId = uVar3;
			continue;
		}
		if (_inventory->getType() != 1) {
			if (_inventory->getType() < 2) {
				if (_inventory->getType() == 0) {
					if ((checkForInventoryButtonRelease() && isInputEnabled()) &&
						((bit_flags_8006fbd8 & 3) != 1)) {
						sequenceId = _dragonVAR->getVar(7);
						uVar7 = _inventory->_old_showing_value;
						_inventory->_old_showing_value = _inventory->getType();
						joined_r0x800271d0:
						_inventory->setType(_inventory->_old_showing_value);
						if (sequenceId == 1) {
							LAB_800279f4:
							_inventory->_old_showing_value = uVar7;
							FUN_8003130c();
							actorId = uVar3;
						}
						else {
							_counter = 0;
							_inventory->setType(1);
							_inventory->openInventory();
							joined_r0x80027a38:
							if (_cursor->iniItemInHand == 0) {
								_cursor->_sequenceID = 1;
								actorId = uVar3;
							}
							else {
								_cursor->_sequenceID = 5;
								actorId = uVar3;
							}
						}
						continue;
					}
					uVar6 = _inventory->getType();
					if (checkForActionButtonRelease() && isFlagSet(ENGINE_FLAG_8)) {
						_counter = 0;
						if ((_cursor->_iniUnderCursor & 0x8000) != 0) {
							if (_cursor->_iniUnderCursor == 0x8002) {
								LAB_80027294:
								uVar7 = 0;
								if (_cursor->iniItemInHand == 0) {
									if ((bit_flags_8006fbd8 & 3) != 1) {
										sequenceId = _dragonVAR->getVar(7);
										uVar7 = _inventory->_old_showing_value;
										_inventory->_old_showing_value = _inventory->getType();
										goto joined_r0x800271d0;
									}
								}
								else {
									actorId = 0;
									do {
										if (unkArray_uint16[actorId] == 0) break;
										uVar7 = uVar7 + 1;
										actorId = (uint)uVar7;
									} while (uVar7 < 0x29);
									if (uVar7 < 0x29) {
										_cursor->_sequenceID = 1;
										waitForFrames(1);
										uVar6 = _cursor->iniItemInHand;
										_cursor->iniItemInHand = 0;
										_cursor->_iniUnderCursor = 0;
										unkArray_uint16[(uint)uVar7] = uVar6;
										actorId = uVar3;
										continue;
									}
								}
							}
							else {
								if (_cursor->_iniUnderCursor != 0x8001) goto LAB_80027ab4;
								if (_inventory->getSequenceId() == 0) goto LAB_80027294;
							}
							if ((_cursor->_iniUnderCursor == 0x8001) && (_inventory->getSequenceId() == 1)) {
								_inventory->setType(2);
								_inventory->_old_showing_value = uVar6;
								FUN_80038890();
								actorId = uVar3;
								continue;
							}
						}
						LAB_80027ab4:
						_counter = 0;
						_cursor->data_80072890 = _cursor->_iniUnderCursor;
						if (_cursor->_sequenceID < 5) {
							_cursor->data_800728b0_cursor_seqID = _cursor->_sequenceID;
							walkFlickerToObject();
							if (bit_flags_8006fbd8 != 0) {
								clearFlags(ENGINE_FLAG_8);
							}
						}
						else {
							_cursor->data_800728b0_cursor_seqID = _cursor->_sequenceID;
							walkFlickerToObject();
							if (bit_flags_8006fbd8 != 0) {
								clearFlags(ENGINE_FLAG_8);
							}
							_scriptOpcodes->_data_800728c0 = _cursor->data_80072890;
							_cursor->data_80072890 = _cursor->iniItemInHand;
						}
					}
				}
			}
			else {
				if (_inventory->getType() == 2) {
					uVar6 = _inventory->getType();
					if (checkForInventoryButtonRelease() && isInputEnabled()) {
						uVar7 = _inventory->_old_showing_value;
						if (_dragonVAR->getVar(7) == 1) goto LAB_800279f4;
						_counter = 0;
						_inventory->setType(1);
						_inventory->_old_showing_value = uVar6;
						_inventory->openInventory();
						goto joined_r0x80027a38;
					}
					if (checkForActionButtonRelease() && isFlagSet(ENGINE_FLAG_8)) goto LAB_80027ab4;
				}
			}
			LAB_80027b58:
			runINIScripts();
			actorId = uVar3;
			continue;
		}
		if (checkForInventoryButtonRelease()) {
			_counter = 0;
			LAB_80027970:
			_inventory->closeInventory();
			uVar6 = _inventory->_old_showing_value;
			_inventory->_old_showing_value = _inventory->getType();
			actorId = uVar3;
			_inventory->setType(uVar6);
			continue;
		}
		uVar6 = _inventory->getType();
		if (checkForActionButtonRelease() && isFlagSet(ENGINE_FLAG_8)) {
			_counter = 0;
			if ((_cursor->_iniUnderCursor & 0x8000) != 0) {
				if (_cursor->_iniUnderCursor == 0x8001) {
					_inventory->closeInventory();
					_inventory->setType(0);
					if (_inventory->_old_showing_value == 2) {
						FUN_80038994();
					}
				}
				else {
					if (_cursor->_iniUnderCursor != 0x8002) goto LAB_8002790c;
					_inventory->closeInventory();
					_inventory->setType(2);
					if (_inventory->_old_showing_value != 2) {
						FUN_80038890();
					}
				}
				_inventory->_old_showing_value = uVar6;
				actorId = uVar3;
				continue;
			}
			if (_cursor->_iniUnderCursor != 0) {
				actorId_00 = 0;
				if ((_cursor->_sequenceID != 4) && (_cursor->_sequenceID != 2)) {
					_cursor->data_800728b0_cursor_seqID = _cursor->_sequenceID;
					_cursor->data_80072890 = _cursor->_iniUnderCursor;
					if (4 < _cursor->_sequenceID) {
						_cursor->data_80072890 = _cursor->iniItemInHand;
						_scriptOpcodes->_data_800728c0 = _cursor->_iniUnderCursor;
					}
					clearFlags(ENGINE_FLAG_8);
					walkFlickerToObject();
					goto LAB_8002790c;
				}
				if (_cursor->_iniUnderCursor != unkArray_uint16[0]) {
					actorId = 1;
					do {
						actorId_00 = actorId;
						actorId = actorId_00 + 1;
					} while (_cursor->_iniUnderCursor != unkArray_uint16[actorId_00 & 0xffff]);
				}
				puVar9 = unkArray_uint16 + (actorId_00 & 0xffff);
				Actor *actor = _actorManager->getActor(actorId_00 + 0x17);
				*puVar9 = _cursor->iniItemInHand;
				_cursor->data_8007283c = actor->_sequenceID;
				actor->clearFlag(ACTOR_FLAG_40);
				_cursor->iniItemInHand = _cursor->_iniUnderCursor;
				_cursor->_sequenceID = 5;
				actorId = uVar3;
				if (*puVar9 != 0) {
					actorId = actorId_00 + 0x17 & 0xffff;
					actor->flags = 0;
					actor->priorityLayer = 0;
					actor->field_e = 0x100;
					actor->updateSequence(getINI((uint)*puVar9 - 1)->field_8 * 2 + 10);
					actor->setFlag(ACTOR_FLAG_40);
					actor->setFlag(ACTOR_FLAG_80);
					actor->setFlag(ACTOR_FLAG_100);
					actor->setFlag(ACTOR_FLAG_200);
					actor->priorityLayer = 6;
					actorId = uVar3;
				}
				continue;
			}
			uVar6 = 0;
			if (_cursor->iniItemInHand == 0) goto LAB_80027b58;
			//drop item back into inventory
			actorId = 0;
			do {
				Actor *actor = _actorManager->getActor(actorId + 0x17);
				if (((((int)(short)actor->x_pos - 0x10 <= (int)_cursor->_x) &&
					  ((int)_cursor->_x < (int)(short)actor->x_pos + 0x10)) &&
					 ((int)(short)actor->y_pos - 0xc <= (int)_cursor->_y)) &&
					(actorId = (uint)uVar6, (int)_cursor->_y < (int)(short)actor->y_pos + 0xc)) {
					break;
				}
				uVar6 = uVar6 + 1;
				actorId = (uint)uVar6;
			} while (uVar6 < 0x29);
			if (actorId != 0x29) {
				actorId_00 = (uint)(ushort)(uVar6 + 0x17);
				unkArray_uint16[actorId] = _cursor->iniItemInHand;
				Actor *actor = _actorManager->getActor(actorId_00);
				actor->flags = 0;
				actor->priorityLayer = 0;
				actor->field_e = 0x100;
				_cursor->iniItemInHand = 0;
				actor->updateSequence(
						 getINI((uint)unkArray_uint16[actorId] - 1)->field_8 * 2 + 10);
				uVar6 = _cursor->_sequenceID;
				actor->setFlag(ACTOR_FLAG_40);
				actor->setFlag(ACTOR_FLAG_80);
				actor->setFlag(ACTOR_FLAG_100);
				actor->setFlag(ACTOR_FLAG_200);
				actor->priorityLayer = 6;
				if (uVar6 == 5) {
					_cursor->_sequenceID = 4;
				}
			}
		}
		LAB_8002790c:
		if ((_cursor->iniItemInHand == 0) ||
			(((ushort)(_cursor->_x - 10U) < 300 && ((ushort)(_cursor->_y - 10U) < 0xb4))))
			goto LAB_80027b58;
		_cursor->_sequenceID = 5;
		waitForFrames(2);
		goto LAB_80027970;
	}
}


void DragonsEngine::updateHandler() {
	data_8006a3a0_flag |= 0x40;
	//TODO logic here

	updateActorSequences();

	_cursor->updateVisibility();
	_inventory->updateVisibility();

	//TODO logic here
	for (uint16 i = 0; i < 0x17; i++) {
		Actor *actor = _actorManager->getActor(i);
		if (actor->flags & Dragons::ACTOR_FLAG_40) {
			if (!(actor->flags & Dragons::ACTOR_FLAG_100)) {
				int16 priority = _scene->getPriorityAtPosition(Common::Point(actor->x_pos, actor->y_pos));
				DragonINI *flicker = _dragonINIResource->getFlickerRecord();
				if (flicker && _scene->contains(flicker) && flicker->actor->_actorID == i) {
					if (priority < 8 || priority == 0x10) {
						actor->priorityLayer = priority;
					}
				} else {
					if (priority != -1) {
						actor->priorityLayer = priority;
					}
				}

				if (actor->priorityLayer >= 0x11) {
					actor->priorityLayer = 0;
				}

				if (actor->priorityLayer >= 9) {
					actor->priorityLayer -= 8;
				}
			}

			if (actor->sequenceTimer != 0) {
				actor->sequenceTimer--;
			}
		}
	}

	if (_flags & Dragons::ENGINE_FLAG_80) {
		for (uint16 i = 0x17; i < DRAGONS_ENGINE_NUM_ACTORS; i++) {
			Actor *actor = _actorManager->getActor(i);
			if (actor->sequenceTimer != 0) {
				actor->sequenceTimer--;
			}
		}
	}

	if (isFlagSet(ENGINE_FLAG_4)) {
		updatePathfindingActors();
	}

	// TODO 0x8001bed0

	// 0x8001c294
	if (!(_unkFlags1 & ENGINE_UNK1_FLAG_8)) {
		//TODO ReadPad();
	}

	if (isFlagSet(ENGINE_FLAG_20)) {
		engineFlag0x20UpdateFunction();
	}

	//TODO vsync update function

	// TODO data_8006a3a0 logic. @ 0x8001c2f4

	data_8006a3a0_flag &= ~0x40;
}

uint32 DragonsEngine::calulateTimeLeft() {
	uint32 now;

	now = _system->getMillis();

	if (_nextUpdatetime <= now) {
		_nextUpdatetime = now + DRAGONS_TICK_INTERVAL;
		return (0);
	}
	uint32 delay = _nextUpdatetime - now;
	_nextUpdatetime += DRAGONS_TICK_INTERVAL;
	return (delay);
}

void DragonsEngine::wait() {
	_system->delayMillis(calulateTimeLeft());
}

void DragonsEngine::updateActorSequences() {
	if (!(_flags & Dragons::ENGINE_FLAG_4)) {
		return;
	}

	//TODO ResetRCnt(0xf2000001);

	int16 actorId = _flags & Dragons::ENGINE_FLAG_80 ? (int16) 64 : (int16) 23;

	while (actorId > 0) {
		actorId--;
		Actor *actor = _actorManager->getActor((uint16) actorId);
		if (actorId < 2 && _flags & Dragons::ENGINE_FLAG_40) {
			continue;
		}

		if (actor->flags & Dragons::ACTOR_FLAG_40 &&
			!(actor->flags & Dragons::ACTOR_FLAG_4) &&
			!(actor->flags & Dragons::ACTOR_FLAG_400) &&
			(actor->sequenceTimer == 0 || actor->flags & Dragons::ACTOR_FLAG_1)) {
			debug(3, "Actor[%d] execute sequenceOp", actorId);

			if (actor->flags & Dragons::ACTOR_FLAG_1) {
				actor->resetSequenceIP();
				//clear flag mask 0xeff6;
				actor->clearFlag(ACTOR_FLAG_1);
				actor->clearFlag(ACTOR_FLAG_8);
				actor->clearFlag(ACTOR_FLAG_1000);
				actor->field_7a = 0;
			}
			OpCall opCall;
			opCall._result = 1;
			while (opCall._result == 1) {
				opCall._op = (byte) READ_LE_UINT16(actor->_seqCodeIp);
				opCall._code = actor->_seqCodeIp + 2;
				_sequenceOpcodes->execOpcode(actor, opCall);
				actor->_seqCodeIp += opCall._deltaOfs;
			}
		}
	}
}

void DragonsEngine::setFlags(uint32 flags) {
	_flags |= flags;
}

void DragonsEngine::clearFlags(uint32 flags) {
	_flags &= ~flags;
}

void DragonsEngine::setUnkFlags(uint32 flags) {
	_unkFlags1 |= flags;
}

void DragonsEngine::clearUnkFlags(uint32 flags) {
	_unkFlags1 &= ~flags;
}

byte *DragonsEngine::getBackgroundPalette() {
	assert(_scene);
	return _scene->getPalette();
}

bool DragonsEngine::isFlagSet(uint32 flag) {
	return (bool) (_flags & flag);
}

bool DragonsEngine::isUnkFlagSet(uint32 flag) {
	return (bool) (_unkFlags1 & flag);
}

DragonINI *DragonsEngine::getINI(uint32 index) {
	return _dragonINIResource->getRecord(index);
}

uint16 DragonsEngine::getVar(uint16 offset) {
	return _dragonVAR->getVar(offset);
}

uint16 DragonsEngine::getCurrentSceneId() {
	return _scene->getSceneId();
}

void DragonsEngine::setVar(uint16 offset, uint16 value) {
	return _dragonVAR->setVar(offset, value);
}

uint16 DragonsEngine::getIniFromImg() {
	DragonINI *flicker = _dragonINIResource->getFlickerRecord();

	int16 x = flicker->actor->x_pos / 32;
	int16 y = flicker->actor->y_pos / 8;

	uint16 currentSceneId = _scene->getSceneId();

	for (uint16 i = 0; i < _dragonINIResource->totalRecords(); i++) {
		DragonINI *ini = getINI(i);
		if (ini->sceneId == currentSceneId && ini->field_1a_flags_maybe == 0) {
			IMG *img = _dragonIMG->getIMG(ini->field_2);
			if (x >= img->x &&
				img->x + img->w >= x &&
				y >= img->y &&
				img->h + img->y >= y) {
				return i + 1;
			}
		}

	}
	return 0;
}

void DragonsEngine::runINIScripts() {
	for (uint16 i = 0; i < _dragonINIResource->totalRecords(); i++) {
		DragonINI *ini = getINI(i);
		if (ini->field_1a_flags_maybe & Dragons::INI_FLAG_10) {
			ini->field_1a_flags_maybe &= ~Dragons::INI_FLAG_10;
			byte *data = _dragonOBD->getFromOpt(i);
			ScriptOpCall scriptOpCall;
			scriptOpCall._code = data + 8;
			scriptOpCall._codeEnd = scriptOpCall._code + READ_LE_UINT32(data);
			uint32 currentFlags = _flags;
			clearFlags(Dragons::ENGINE_FLAG_8);
			_scriptOpcodes->runScript3(scriptOpCall);
			_flags = currentFlags;
		}
	}
}
/*
void DragonsEngine::engineFlag0x20UpdateFunction() {
{
	ushort uVar1;
	uint16_t uVar2;
	DragonINI *pDVar3;
	DragonINI *pDVar4;
	ushort uVar5;
	uint actorId;

	DragonINI *flickerINI = _dragonINIResource->getFlickerRecord();
	uint16 currentSceneId = _scene->getSceneId();


//	uVar1 = flickerINI->actorId;
//	actorId = (uint)uVar1;
//	uVar5 = dragon_ini_pointer[dragon_ini_const__2 + -1].field_0x1c;
	if (flickerINI == NULL) {
		//LAB_80027d40:
//		if ((flickerINI->sceneId == currentSceneId)
//			&& (uVar5 != 0xffff)) {
//			actors[(uint)uVar5].﻿_sequenceID = 8;
//			actors[(uint)uVar5].﻿priorityLayer_maybe = 0;
//		}
	}
	else {
		if (flickerINI->sceneId == currentSceneId) {
			if ((flickerINI == NULL) || flickerINI->actor->isFlagSet(ACTOR_FLAG_10)) {
				if ((flickerINI->sceneId == currentSceneId)
					&& (uVar5 != 0xffff)) {
//					actors[(uint)uVar5].﻿_sequenceID = 8;
//					actors[(uint)uVar5].﻿priorityLayer_maybe = 0;
				}
			} else {
				if ((bit_flags_8006fbd8 & 2) == 0) {
					bit_flags_8006fbd8 = bit_flags_8006fbd8 | 2;
				}
//				if (((((actors[actorId].flags & 0x2000) == 0) && ((actors[actorId].flags & 4) != 0)) &&
//					 (actors[actorId].﻿_sequenceID2 != actors[actorId].﻿_sequenceID)) &&
//				(actors[actorId].﻿_sequenceID2 != -1)) {
//					actor_update_sequenceID(actorId,actors[actorId].﻿_sequenceID2);
//				}
			}

		} else {
			//actors[(uint)uVar5].﻿priorityLayer_maybe = 0;
		}

	}


	LAB_80027db4:
	uVar1 = num_ini_records_maybe;
	pDVar3 = dragon_ini_pointer;
	uVar2 = currentSceneId;
	if ((inventoryType == 0) && (uVar5 = 0, num_ini_records_maybe != 0)) {
		actorId = 0;
		do {
			pDVar4 = pDVar3 + actorId;
			if (((-1 < (int)((uint)pDVar4->field_0x10 << 0x10)) && (pDVar4->sceneId_maybe == uVar2)) &&
				(pDVar4->field_0x10 = pDVar4->field_0x10 - 1, (int)((uint)pDVar4->field_0x10 << 0x10) < 0))
			{
				pDVar4->field_1a_flags_maybe = pDVar4->field_1a_flags_maybe | 0x10;
			}
			uVar5 = uVar5 + 1;
			actorId = (uint)uVar5;
		} while (uVar5 < uVar1);
	}
	if (run_func_ptr_unk_countdown_timer != 0) {
		run_func_ptr_unk_countdown_timer = run_func_ptr_unk_countdown_timer - 1;
	}
	return (uint)run_func_ptr_unk_countdown_timer;
} */

void DragonsEngine::engineFlag0x20UpdateFunction() {
	if (_flags & Dragons::ENGINE_FLAG_20) {
		if ((_flags & (Dragons::ENGINE_FLAG_80000000 | Dragons::ENGINE_FLAG_8)) == 8) {
			_cursor->update();
		}
		//TODO 0x80027be4

		uint16 currentSceneId = _scene->getSceneId();
		DragonINI *flickerINI = _dragonINIResource->getFlickerRecord();


//	uVar1 = flickerINI->actorId;
//	actorId = (uint)uVar1;
//	uVar5 = dragon_ini_pointer[dragon_ini_const__2 + -1].field_0x1c;
		if (flickerINI == NULL) {
			//LAB_80027d40:
			//error("LAB_80027d40"); //TODO is this logic required?
//		if ((flickerINI->sceneId == currentSceneId)
//			&& (uVar5 != 0xffff)) {
//			actors[(uint)uVar5].﻿_sequenceID = 8;
//			actors[(uint)uVar5].﻿priorityLayer_maybe = 0;
//		}
		}
		else {
			if (flickerINI->sceneId == currentSceneId) {
				if (flickerINI->actor->isFlagSet(ACTOR_FLAG_10)) {
					if (_inventory->isActorSet()) {
						_inventory->setActorSequenceId(8);
						_inventory->setPriority(0);
					}
				} else {
					if ((bit_flags_8006fbd8 & 2) == 0) {
						bit_flags_8006fbd8 = bit_flags_8006fbd8 | 2;
					}
					if (flickerINI->actor->isFlagClear(ACTOR_FLAG_2000)
					&& flickerINI->actor->isFlagSet(ACTOR_FLAG_4)
					&& flickerINI->actor->_sequenceID2 != -1
					&& flickerINI->actor->_sequenceID2 != flickerINI->actor->_sequenceID) {
						flickerINI->actor->updateSequence(flickerINI->actor->_sequenceID2);
					}
				}
			} else {
				_inventory->setPriority(0); //TODO I don't think this is quite right.
			}

		}

		// 0x80027db8
		if (!_inventory->isVisible()) {
			for (uint16 i = 0; i < _dragonINIResource->totalRecords(); i++) {
				DragonINI *ini = getINI(i);
				if (ini->field_10 >= 0 && ini->sceneId == currentSceneId) {
					ini->field_10--;
					if (ini->field_10 < 0) {
						ini->field_1a_flags_maybe |= Dragons::INI_FLAG_10;
					}
				}
			}
		}

		if (run_func_ptr_unk_countdown_timer != 0) {
			run_func_ptr_unk_countdown_timer--;
		}
	}
}

void DragonsEngine::waitForFrames(uint16 numFrames) {
	for (uint16 i = 0; i < numFrames; i++) {
		wait();
		updateHandler();

		_scene->draw();
		_screen->updateScreen();
		updateEvents();
	}
}

void DragonsEngine::playSound(uint16 soundId) {
	debug(3, "TODO: play sound %d", soundId);
}

void DragonsEngine::updatePathfindingActors() {
	for (uint16 i = 0; i < 0x17; i++) {
		Actor *actor = _actorManager->getActor(i);
		actor->walkPath();
	}
}

void DragonsEngine::fade_related(uint32 flags) {
	if (!isFlagSet(ENGINE_FLAG_40)) {
		return;
	}
	setUnkFlags(ENGINE_UNK1_FLAG_2);
	clearFlags(ENGINE_FLAG_40);

	//TODO 0x80015a1c
}

void DragonsEngine::call_fade_related_1f() {
	fade_related(0x1f);
}

void DragonsEngine::works_with_obd_data_1() {
	uint uVar1;
	ushort uVar2;
	uint uVar4;
	uint uVar5;
	uint uVar6;
	ScriptOpCall local_48;
	ScriptOpCall local_38;
	ScriptOpCall local_58;
	byte * pvVar7;
	byte * pvVar8;

	uVar2 = _scriptOpcodes->_data_800728c0;
	uVar1 = _flags;
	local_58._code = NULL;
	local_58._codeEnd = NULL;
	local_58._result = 0;

	uVar6 = 0;
	_scriptOpcodes->_data_80071f5c = 0;

	assert(_cursor->data_80072890 > 0);
	byte *obd = _dragonOBD->getFromOpt(_cursor->data_80072890 - 1);


	local_48._code = pvVar7 = obd + 8;
	local_48._codeEnd = pvVar8 = local_48._code + READ_LE_UINT32(obd);

	uVar4 = _cursor->executeScript(local_48, 1);
	if (_cursor->data_800728b0_cursor_seqID > 4) {
		_scriptOpcodes->_data_80071f5c = 0;
		_scriptOpcodes->_data_800728c0 = _cursor->data_80072890;

		obd = _dragonOBD->getFromOpt(_scriptOpcodes->_data_800728c0 - 1);

		local_38._code = obd + 8;
		local_38._codeEnd = local_38._code + READ_LE_UINT32(obd);

		uVar6 = _cursor->executeScript(local_38, 1);
		_scriptOpcodes->_data_800728c0 = uVar2;
	}
	if ((uVar6 & 0xffff) != 0) {
		local_58._code = local_38._code + 8;
		local_58._codeEnd = local_58._code + READ_LE_UINT16(local_38._code + 6);
	}
	if (((uVar4 & 0xffff) != 0) && ((((uVar4 & 2) == 0 || ((uVar6 & 2) != 0)) || ((uVar6 & 0xffff) == 0)))) {
		local_58._code = local_48._code + 8;
		local_58._codeEnd = local_58._code + READ_LE_UINT16(local_48._code + 6);
	}
	uVar4 = uVar4 & 0xfffd;
	if (local_58._code != NULL && local_58._codeEnd != NULL) {
		clearFlags(ENGINE_FLAG_8);
		_scriptOpcodes->runScript(local_58);
	}
	if ((local_58._result & 1U) == 0) {
		if (_cursor->data_800728b0_cursor_seqID == 3) {
			local_58._code = pvVar7;
			local_58._codeEnd = pvVar8;
			uVar5 = _scriptOpcodes->FUN_800297d8(local_58);
			uVar4 = uVar4 | uVar5;
		}
		if (((uVar4 & 0xffff) == 0) && ((uVar6 & 0xfffd) == 0)) {
			FUN_8002931c();
		}
	}
	else {
		_scriptOpcodes->_data_80071f5c--;
	}
	_flags |= uVar1 & ENGINE_FLAG_8;
	return;
}

bool DragonsEngine::checkForInventoryButtonRelease() {
	return _iKeyUp;
}

bool DragonsEngine::isInputEnabled() {
	return !isFlagSet(ENGINE_FLAG_20000000) && !isFlagSet(ENGINE_FLAG_400);
}

bool DragonsEngine::checkForActionButtonRelease() {
	return _leftMouseButtonUp;
}

void DragonsEngine::FUN_8003130c() {
	error("FUN_8003130c"); //TODO
}

void DragonsEngine::FUN_80038890() {
	error("FUN_80038890"); //TODO
}

void DragonsEngine::walkFlickerToObject()
{
	ushort targetX;
	ushort targetY;
	uint uVar7;
	uint uVar8;
	DragonINI *targetINI;
	DragonINI *flickerINI;

	flickerINI = _dragonINIResource->getFlickerRecord();
	if (flickerINI->sceneId == getCurrentSceneId()) {
		if (_cursor->data_80072890 != 0) {

			if (!(READ_LE_UINT16(_dragonOBD->getFromOpt(_cursor->data_80072890 - 1) + 4) & 8)
			&& (_inventory->getType() == 0) && !isFlagSet(ENGINE_FLAG_200000)) {
				targetINI = getINI(_cursor->data_80072890 - 1);
				if ((targetINI->field_1a_flags_maybe & 1) == 0) {
					if (targetINI->actorResourceId == -1) {
						return;
					}
					IMG *img = _dragonIMG->getIMG(targetINI->field_2);
					targetX = img->field_a;
					targetY = img->field_c;
				}
				else {
					targetX = targetINI->actor->x_pos;
					targetY = targetINI->actor->y_pos;
				}
				flickerINI->actor->field_7c = 0x10000;
				if (flickerINI->field_20_actor_field_14 == -1) {
					flickerINI->actor->setFlag(ACTOR_FLAG_800);
				}
				flickerINI->actor->pathfinding_maybe((int)(((uint)targetX + (uint)targetINI->field_1c) * 0x10000) >> 0x10,
													(int)(((uint)targetY + (uint)targetINI->field_1e) * 0x10000) >> 0x10,0);
				bit_flags_8006fbd8 = 1;
				return;
			}
			if (isFlagSet(ENGINE_FLAG_200000)) {
				bit_flags_8006fbd8 = 3;
				return;
			}
			flickerINI = _dragonINIResource->getFlickerRecord();
			if (flickerINI != NULL && flickerINI->actor != NULL) {
				flickerINI->actor->clearFlag(ACTOR_FLAG_10);
				uVar8 = (uint)_cursor->data_80072890;
				flickerINI->actor->setFlag(ACTOR_FLAG_4);
				targetINI = getINI(_cursor->data_80072890 - 1);
				flickerINI->field_20_actor_field_14 = targetINI->field_e;
				flickerINI->actor->_sequenceID2 = targetINI->field_e;
			}
			bit_flags_8006fbd8 = 3;
			return;
		}
		if (_inventory->getType() == 0 && !isFlagSet(ENGINE_FLAG_200000)) {
			uVar7 = (uint)(ushort)_cursor->_x;
			uVar8 = (uint)(ushort)_cursor->_y;
			flickerINI->actor->field_7c = 0x10000;
			flickerINI->actor->pathfinding_maybe(
					(int)((uVar7 + (uint)_scene->_camera.x) * 0x10000) >> 0x10,
					(int)((uVar8 + (uint)_scene->_camera.y) * 0x10000) >> 0x10,0);
		}
	}
	else {
		if (_cursor->data_80072890 != 0) {
			bit_flags_8006fbd8 = 3;
			return;
		}
	}
	bit_flags_8006fbd8 = 0;
	return;
}

void DragonsEngine::FUN_80038994() {
	error("FUN_80038994"); //TODO
}

void DragonsEngine::FUN_8002931c() {

	DragonINI *flicker = _dragonINIResource->getFlickerRecord();
	if (flicker && flicker->actor) {
		flicker->actor->clearFlag(ACTOR_FLAG_10);
		if (getCurrentSceneId() != 0x2e || !flicker->actor->_actorResource || flicker->actor->_actorResource->_id != 0x91) {
			flicker->actor->setFlag(ACTOR_FLAG_4);
		}
	}
//TODO
//	uVar1 = FUN_80023830(9);
//	local_30 = 0x11;
//	local_2c = local_280[(uint)DAT_800728b0 * 9 + (uVar1 & 0xffff)];
//	FlickerTalkMaybe(&local_30);

error("FUN_8002931c"); //TODO
}

void DragonsEngine::reset_screen_maybe() {
	data_8006a3a0_flag &= ~0x10;
	//TODO
}

bool DragonsEngine::canLoadGameStateCurrently() {
	return isInputEnabled();
}

bool DragonsEngine::canSaveGameStateCurrently() {
	return isInputEnabled() && _inventory->getType() != 1;
}

bool DragonsEngine::hasFeature(Engine::EngineFeature f) const {
	return
		// TODO (f == kSupportsRTL) ||
		(f == kSupportsLoadingDuringRuntime) ||
		(f == kSupportsSavingDuringRuntime);
}

void DragonsEngine::loadScene(uint16 sceneId) {
	_flags = 0x1046;
	_flags &= 0x1c07040;
	_flags |= 0x26;
	_unkFlags1 = 0;

	_scriptOpcodes->_data_800728c0 = 0; //TODO this should be reset in scriptopcode.
	_cursor->init(_actorManager, _dragonINIResource);
	_inventory->init(_actorManager, _backgroundResourceLoader, new Bag(_bigfileArchive, _screen), _dragonINIResource);

	if (sceneId > 2) {
		_dragonVAR->setVar(1, 1);
	}

	if (sceneId > 2) { //TODO remove this restriction to enable intro sequence.
		_scene->setSceneId(2);
		byte *obd = _dragonOBD->getFromSpt(3);
		ScriptOpCall scriptOpCall;
		scriptOpCall._code = obd + 4;
		scriptOpCall._codeEnd = scriptOpCall._code + READ_LE_UINT32(obd);
		_scriptOpcodes->runScript(scriptOpCall);
	} else {
		sceneId = 0x12; // HACK the first scene. TODO remove this
	}

	if(getINI(0)->sceneId == 0) {
		getINI(0)->sceneId = sceneId; //TODO
	} else {
		_scene->setSceneId(getINI(0)->sceneId);
	}
	_sceneId1 = sceneId;
	_scene->loadScene(sceneId, 0x1e);
}

void DragonsEngine::reset() {
	_nextUpdatetime = 0;
	_flags = 0;
	_unkFlags1 = 0;
	run_func_ptr_unk_countdown_timer = 0;
	data_8006a3a0_flag = 0;
	data_800633fa = 0;

	for(int i = 0; i < 8; i++) {
		opCode1A_tbl[i].field0 = 0;
		opCode1A_tbl[i].field2 = 0;
		opCode1A_tbl[i].field4 = 0;
		opCode1A_tbl[i].field6 = 0;
		opCode1A_tbl[i].field8 = 0;
	}

	memset(unkArray_uint16, 0, sizeof(unkArray_uint16));

}

} // End of namespace Dragons