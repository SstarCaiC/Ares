#include "UnitDelivery.h"
#include "../../Ext/Techno/Body.h"
#include "../../Utilities/TemplateDef.h"

#include <AircraftClass.h>
#include <BuildingClass.h>
#include <CellSpread.h>
#include <HouseClass.h>
#include <OverlayTypeClass.h>

void SW_UnitDelivery::Initialize(SWTypeExt::ExtData *pData, SuperWeaponTypeClass *pSW)
{
	pData->SW_AITargetingType = SuperWeaponAITargetingMode::ParaDrop;
}

void SW_UnitDelivery::LoadFromINI(SWTypeExt::ExtData *pData, SuperWeaponTypeClass *pSW, CCINIClass *pINI)
{
	const char * section = pSW->ID;

	if(!pINI->GetSection(section)) {
		return;
	}

	INI_EX exINI(pINI);
	pData->SW_Deliverables.Read(exINI, section, "Deliver.Types");
	pData->SW_DeliverBuildups.Read(exINI, section, "Deliver.Buildups");
	pData->SW_OwnerHouse.Read(exINI, section, "Deliver.Owner");
}

bool SW_UnitDelivery::Activate(SuperClass* pThis, const CellStruct &Coords, bool IsPlayer)
{
	SuperWeaponTypeClass *pSW = pThis->Type;
	SWTypeExt::ExtData *pData = SWTypeExt::ExtMap.Find(pSW);

	int deferment = pData->SW_Deferment.Get(-1);
	if(deferment < 0) {
		deferment = 20;
	}

	this->newStateMachine(deferment, Coords, pThis);

	return true;
}

void UnitDeliveryStateMachine::Update()
{
	if(this->Finished()) {
		CoordStruct coords = CellClass::Cell2Coord(this->Coords);

		auto pData = this->FindExtData();

		pData->PrintMessage(pData->Message_Activate, this->Super->Owner);

		if(pData->SW_ActivationSound != -1) {
			VocClass::PlayAt(pData->SW_ActivationSound, coords, nullptr);
		}

		this->PlaceUnits();
	}
}

//This function doesn't skip any placeable items, no matter how
//they are ordered. Infantry is grouped. Units are moved to the
//center as close as they can.

void UnitDeliveryStateMachine::PlaceUnits()
{
	auto pData = this->FindExtData();

	if(!pData) {
		return;
	}

	// get the house the units will belong to
	auto pOwner = this->Super->Owner;
	if(pData->SW_OwnerHouse == OwnerHouseKind::Civilian) {
		pOwner = HouseClass::FindCivilianSide();
	} else if(pData->SW_OwnerHouse == OwnerHouseKind::Special) {
		pOwner = HouseClass::FindSpecial();
	} else if(pData->SW_OwnerHouse == OwnerHouseKind::Neutral) {
		pOwner = HouseClass::FindNeutral();
	}

	// create an instance of each type and place it
	for(auto Type : pData->SW_Deliverables) {
		auto Item = abstract_cast<TechnoClass*>(Type->CreateObject(pOwner));
		auto ItemBuilding = abstract_cast<BuildingClass*>(Item);

		// get the best options to search for a place
		int extentX = 1;
		int extentY = 1;
		SpeedType::Value SpeedType = SpeedType::Track;
		MovementZone::Value MovementZone = MovementZone::Normal;

		if(ItemBuilding) {
			extentX = ItemBuilding->Type->GetFoundationWidth();
			extentY = ItemBuilding->Type->GetFoundationHeight(true);
		} else {
			// place aircraft types on ground explicitly
			if(Type->WhatAmI() != abs_AircraftType) {
				SpeedType = Type->SpeedType;
				MovementZone = Type->MovementZone;
			}
		}

		// find a place to put this
		int a5 = -1; // usually MapClass::CanLocationBeReached call. see how far we can get without it
		auto PlaceCoords = MapClass::Instance->Pathfinding_Find(this->Coords,
			SpeedType, a5, MovementZone, false, extentX, extentY, true, false,
			false, false, CellStruct::Empty, false, ItemBuilding != nullptr);

		if(auto pCell = MapClass::Instance->TryGetCellAt(PlaceCoords)) {
			Item->OnBridge = pCell->ContainsBridge();

			// set the appropriate mission
			if(ItemBuilding && pData->SW_DeliverBuildups) {
				ItemBuilding->QueueMission(mission_Construction, false);
			} else {
				// only computer units can hunt
				auto Guard = ItemBuilding || pOwner->ControlledByHuman();
				auto Mission = Guard ? mission_Guard : mission_Hunt;
				Item->QueueMission(Mission, false);
			}

			// place and set up
			auto XYZ = pCell->GetCoordsWithBridge();
			if(Item->Put(XYZ, (MapClass::GetCellIndex(pCell->MapCoords) & 7))) {
				if(ItemBuilding) {
					if(pData->SW_DeliverBuildups) {
						ItemBuilding->DiscoveredBy(this->Super->Owner);
						ItemBuilding->unknown_bool_6DD = 1;
					}
				} else if(Type->BalloonHover || Type->JumpJet) {
					Item->Scatter(CoordStruct::Empty, true, false);
				}

				if(auto pItemData = TechnoExt::ExtMap.Find(Item)) {
					if(!pItemData->IsPowered() || !pItemData->IsOperated()) {
						Item->Deactivate();
						if(ItemBuilding) {
							Item->Owner->RecheckTechTree = true;
						}
					}
				}
			} else {
				Item->UnInit();
			}
		}
	}
}
