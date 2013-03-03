
#include "RoF2.h"
#include "../opcodemgr.h"
#include "../logsys.h"
#include "../EQStreamIdent.h"

#include "../eq_packet_structs.h"
#include "RoF2_structs.h"

namespace RoF2 {

static const char *name = "RoF2";
static OpcodeManager *opcodes = NULL;
static Strategy struct_strategy;
	

void Register(EQStreamIdentifier &into) {
	//create our opcode manager if we havent already
	//if(opcodes == NULL) {
		//TODO: get this file name from the config file
		string opfile = "patch_";
		opfile += name;
		opfile += ".conf";
		//load up the opcode manager.
		//TODO: figure out how to support shared memory with multiple patches...
		opcodes = new RegularOpcodeManager();
		if(!opcodes->LoadOpcodes(opfile.c_str())) {
			_log(NET__OPCODES, "Error loading opcodes file %s. Not registering patch %s.", opfile.c_str(), name);
			return;
		}
	//}
	
	//ok, now we have what we need to register.
	
	EQStream::Signature signature;
	string pname;
	
	//register our world signature.
	pname = string(name) + "_world";
	signature.ignore_eq_opcode = 0;
	signature.first_length = sizeof(structs::LoginInfo_Struct);
	signature.first_eq_opcode = opcodes->EmuToEQ(OP_SendLoginInfo);
	into.RegisterPatch(signature, pname.c_str(), &opcodes, &struct_strategy);
	
	//register our zone signature.
	pname = string(name) + "_zone";
	signature.ignore_eq_opcode = opcodes->EmuToEQ(OP_AckPacket);
	signature.first_length = sizeof(structs::ClientZoneEntry_Struct);
	signature.first_eq_opcode = opcodes->EmuToEQ(OP_ZoneEntry);
	into.RegisterPatch(signature, pname.c_str(), &opcodes, &struct_strategy);
	
	
	
	_log(NET__IDENTIFY, "Registered patch %s", name);
}

void Reload() {
	
	//we have a big problem to solve here when we switch back to shared memory
	//opcode managers because we need to change the manager pointer, which means
	//we need to go to every stream and replace it's manager.
	
	if(opcodes != NULL) {
		//TODO: get this file name from the config file
		string opfile = "patch_";
		opfile += name;
		opfile += ".conf";
		if(!opcodes->ReloadOpcodes(opfile.c_str())) {
			_log(NET__OPCODES, "Error reloading opcodes file %s for patch %s.", opfile.c_str(), name);
			return;
		}
		_log(NET__OPCODES, "Reloaded opcodes for patch %s", name);
	}
}



Strategy::Strategy()
: RoF::Strategy()
{
	//all opcodes default to passthrough.
	#include "SSRegister.h"
	#include "RoF2_ops.h"
}

std::string Strategy::Describe() const {
	std::string r;
	r += "Patch ";
	r += name;
	return(r);
}


#include "SSDefine.h"

ENCODE(OP_HPUpdate)
{
	_log(NET__OPCODES, "This is a RoF2 ENCODE.");
	SETUP_DIRECT_ENCODE(SpawnHPUpdate_Struct, structs::SpawnHPUpdate_Struct);
	OUT(spawn_id);
	OUT(cur_hp);
	OUT(max_hp);
	FINISH_ENCODE();
}



} //end namespace RoF2






