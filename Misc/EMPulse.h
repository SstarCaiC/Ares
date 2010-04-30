#ifndef NEW_EMPULSE_H
#define NEW_EMPULSE_H

#include "../Ext/Techno/Body.h"
#include "../Ext/TechnoType/Body.h"
#include "../Ext/WarheadType/Body.h"

class EMPulse
{
public:
	static void CreateEMPulse(EMPulseClass *Legacy, TechnoClass *Firer);
	static void CreateEMPulse(WarheadTypeExt::ExtData *Warhead, CellStruct Target, TechnoClass *Firer);
	static void DisableEMPEffect(TechnoClass *Techno);
	static bool IsTypeEMPProne(TechnoTypeClass *Type);

protected:
	static bool isEMPTypeImmune(TechnoClass *);
	static bool isEMPImmune(TechnoClass *, HouseClass *);
	static bool isCurrentlyEMPImmune(TechnoClass *, HouseClass *);
	static bool isEligibleEMPTarget(TechnoClass *, HouseClass *, WarheadTypeClass *);
	static int getCappedDuration(int, int, int);
	static void updateRadarBlackout(TechnoClass *);
	static bool enableEMPEffect(TechnoClass *, ObjectClass *);
	static void announceAttack(TechnoClass *);
	static bool verbose;
};

#endif