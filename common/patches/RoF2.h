#ifndef RoF2_H_
#define RoF2_H_

#include "../StructStrategy.h"
#include "RoF.h"

class EQStreamIdentifier;

namespace RoF2 {
	
	//these are the only public member of this namespace.
	extern void Register(EQStreamIdentifier &into);
	extern void Reload();
	
	
	
	//you should not directly access anything below.. 
	//I just dont feel like making a seperate header for it.
	
	//class Strategy : public StructStrategy {
	class Strategy : public RoF::Strategy {
	public:
		Strategy();
		
	protected:
		
		virtual std::string Describe() const;
		
		//magic macro to declare our opcodes
		#include "SSDeclare.h"
		#include "RoF2_ops.h"
		
	};
	
};



#endif /*RoF2_H_*/
