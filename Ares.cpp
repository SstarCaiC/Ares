#include "Ares.h"
#include "Commands/Commands.h"
#include <CommandClass.h>
//include "CallCenter.h"
#include <StaticInits.cpp>

#include "Misc/Debug.h"

//Init Statics
HANDLE  Ares::hInstance = 0;
bool	Ares::bNoLogo = false;
bool	Ares::bNoCD = false;

DWORD Ares::readLength = BUFLEN;
char Ares::readBuffer[BUFLEN];
const char Ares::readDelims[4] = ",";
const char Ares::readDefval[4] = "";

int FrameStepCommandClass::ArmageddonState = 0;

//Implementations
eMouseEventFlags __stdcall Ares::MouseEvent(Point2D* pClient,eMouseEventFlags EventFlags)
{
	return EventFlags;
}

void __stdcall Ares::RegisterCommands()
{
	CommandClass::Array->AddItem(new AIControlCommandClass());
	CommandClass::Array->AddItem(new MapSnapshotCommandClass());
	CommandClass::Array->AddItem(new TestSomethingCommandClass());
	CommandClass::Array->AddItem(new FrameByFrameCommandClass());
	CommandClass::Array->AddItem(new FrameStepCommandClass());
	CommandClass::Array->AddItem(new FirestormToggleCommandClass());
	CommandClass::Array->AddItem(new DumperTypesCommandClass());
}

void __stdcall Ares::CmdLineParse(char** ppArgs,int nNumArgs)
{
	char* pArg;

	Debug::bLog = false;
	bNoCD = false;
	bNoLogo = false;

	// > 1 because the exe path itself counts as an argument, too!
	if(nNumArgs > 1) {
		for(int i = 1; i < nNumArgs; i++) {
			pArg = ppArgs[i];
			_strupr(pArg);
			if(_strcmpi(pArg,"-LOG") == 0) {
				Debug::bLog = true;
			}

			if(_strcmpi(pArg,"-CD") == 0) {
				bNoCD = true;
			}

			if(_strcmpi(pArg,"-NOLOGO") == 0) {
				bNoLogo = true;
			}
		}
	}

	if(!Debug::bLog) {
		Debug::LogFileRemove();
	}
}

void __stdcall Ares::PostGameInit()
{

}

void __stdcall Ares::ExeRun()
{
	Ares::readLength = BUFLEN;
	Debug::LogFileOpen();

	Unsorted::Savegame_Magic = SAVEGAME_MAGIC;
}

void __stdcall Ares::ExeTerminate()
{
//	Debug::LogFileClose();
}

//A new SendPDPlane function
//Allows vehicles, sends one single plane for all types
void Ares::SendPDPlane(HouseClass* pOwner, CellClass* pTarget, AircraftTypeClass* pPlaneType,
		TypeList<TechnoTypeClass*>* pTypes, TypeList<int>* pNums)
{
	if(pNums && pTypes &&
		pNums->Count == pTypes->Count &&
		pNums->Count > 0 &&
		pOwner && pPlaneType && pTarget)
	{
		++Unsorted::SomeMutex;
		AircraftClass* pPlane = (AircraftClass*)pPlaneType->CreateObject(pOwner);
		--Unsorted::SomeMutex;

		pPlane->Spawned = true;

		//Get edge (direction for plane to come from)
		int edge = pOwner->StartingEdge;
		if(edge < edge_NORTH || edge > edge_WEST) {
			edge = pOwner->Edge;
			if(edge < edge_NORTH || edge > edge_WEST) {
				edge = edge_NORTH;
			}
		}

		//some ASM magic, seems to retrieve a random cell struct at a given edge
		CellStruct spawn_cell;

		MapClass::Global()->PickCellOnEdge(&spawn_cell, edge, (CellStruct *)0xB04C38, (CellStruct *)0xB04C38, 4, 1, 0);

		pPlane->QueueMission(mission_ParadropApproach, false);

		if(pTarget) {
			pPlane->SetTarget(pTarget);
		}

		CoordStruct spawn_crd = {(spawn_cell.X << 8) + 128, (spawn_cell.Y << 8) + 128, 0};

		++Unsorted::SomeMutex;
		bool bSpawned = pPlane->Put(&spawn_crd, dir_N);
		--Unsorted::SomeMutex;

		if(bSpawned) {
			pPlane->HasPassengers = true;
			for(int i = 0; i < pTypes->Count; i++) {
				TechnoTypeClass* pTechnoType = pTypes->GetItem(i);

				//only allow infantry and vehicles
				eAbstractType WhatAmI = pTechnoType->WhatAmI();
				if(WhatAmI == abs_UnitType || WhatAmI == abs_InfantryType) {
					for(int k = 0; k < pNums->Items[i]; k++) {
						FootClass* pNew = (FootClass*)pTechnoType->CreateObject(pOwner);
						pNew->Remove();
						pPlane->get_Passengers()->AddPassenger(pNew);
					}
				}
			}
			pPlane->NextMission();
		} else {
			if(pPlane) {
				delete pPlane;
			}
		}
	}
}

//DllMain
bool __stdcall DllMain(HANDLE hInstance,DWORD dwReason,LPVOID v)
{
	if(dwReason==DLL_PROCESS_ATTACH)
	{
		Ares::hInstance = hInstance;
		Debug::LogFileOpen();
//		CallCenter::Init();
	}

	return true;
}

//Exports

//Hook at 0x52C5E0
DEFINE_HOOK(52C5E0, Ares_NoLogo, 7)
{
	if(Ares::bNoLogo)
		return 0x52C5F3;
	else
		return 0;
}

//0x6AD0ED
DEFINE_HOOK(6AD0ED, Ares_AllowSinglePlay, 5)
{
	return 0x6AD16C;
}

	// 55AFB3, 6
DEFINE_HOOK(55AFB3, Armageddon_Advance, 6)
{
	switch(FrameStepCommandClass::ArmageddonState)
	{
		case 1:
			Unsorted::ArmageddonMode = 0;
			FrameStepCommandClass::ArmageddonState = 2;
			break;
		case 2:
			Unsorted::ArmageddonMode = 1;
			FrameStepCommandClass::ArmageddonState = 0;
			break;
		default:
			break;
	}
	return 0;
}

DEFINE_HOOK(7CD810, ExeRun, 9)
{
	Ares::ExeRun();
	return 0;
}

DEFINE_HOOK(7CD8EF, ExeTerminate, 9)
{
	Ares::ExeTerminate();
	return 0;
}

DEFINE_HOOK(52CAE9, _YR_PostGameInit, 5)
{
	Ares::PostGameInit();
	return 0;
}

DEFINE_HOOK(52F639, _YR_CmdLineParse, 5)
{
	GET(char**, ppArgs, ESI);
	GET(int, nNumArgs, EDI);

	Ares::CmdLineParse(ppArgs,nNumArgs);
	return 0;
}

DEFINE_HOOK(533058, CommandClassCallback_Register, 7)
{
	Ares::RegisterCommands();

	R->set_EAX((DWORD)(new DWORD(1)));	//Allocate SetUnitTabCommandClass
	return 0x533062;
}
