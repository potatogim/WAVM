#include "Inline/BasicTypes.h"
#include "Inline/Floats.h"
#include "Logging/Logging.h"
#include "Intrinsics.h"
#include "RuntimePrivate.h"

#include <math.h>

namespace Runtime
{
	DEFINE_INTRINSIC_MODULE(wavmIntrinsics)

	template<typename Float>
	Float quietNaN(Float value)
	{
		Floats::FloatComponents<Float> components;
		components.value = value;
		components.bits.significand |= typename Floats::FloatComponents<Float>::Bits(1) << (Floats::FloatComponents<Float>::numSignificandBits - 1);
		return components.value;
	}

	template<typename Float>
	Float floatMin(Float left,Float right)
	{
		// If either operand is a NaN, convert it to a quiet NaN and return it.
		if(left != left) { return quietNaN(left); }
		else if(right != right) { return quietNaN(right); }
		// If either operand is less than the other, return it.
		else if(left < right) { return left; }
		else if(right < left) { return right; }
		else
		{
			// Finally, if the operands are apparently equal, compare their integer values to distinguish -0.0 from +0.0
			Floats::FloatComponents<Float> leftComponents;
			leftComponents.value = left;
			Floats::FloatComponents<Float> rightComponents;
			rightComponents.value = right;
			return leftComponents.bitcastInt < rightComponents.bitcastInt ? right : left;
		}
	}
	
	template<typename Float>
	Float floatMax(Float left,Float right)
	{
		// If either operand is a NaN, convert it to a quiet NaN and return it.
		if(left != left) { return quietNaN(left); }
		else if(right != right) { return quietNaN(right); }
		// If either operand is less than the other, return it.
		else if(left > right) { return left; }
		else if(right > left) { return right; }
		else
		{
			// Finally, if the operands are apparently equal, compare their integer values to distinguish -0.0 from +0.0
			Floats::FloatComponents<Float> leftComponents;
			leftComponents.value = left;
			Floats::FloatComponents<Float> rightComponents;
			rightComponents.value = right;
			return leftComponents.bitcastInt > rightComponents.bitcastInt ? right : left;
		}
	}

	template<typename Float>
	Float floatCeil(Float value)
	{
		if(value != value) { return quietNaN(value); }
		else { return ceil(value); }
	}
	
	template<typename Float>
	Float floatFloor(Float value)
	{
		if(value != value) { return quietNaN(value); }
		else { return floor(value); }
	}
	
	template<typename Float>
	Float floatTrunc(Float value)
	{
		if(value != value) { return quietNaN(value); }
		else { return trunc(value); }
	}
	
	template<typename Float>
	Float floatNearest(Float value)
	{
		if(value != value) { return quietNaN(value); }
		else { return nearbyint(value); }
	}

	DEFINE_INTRINSIC_FUNCTION2(wavmIntrinsics,f32Min,f32.min,f32,f32,left,f32,right) { return floatMin(left,right); }
	DEFINE_INTRINSIC_FUNCTION2(wavmIntrinsics,f64Min,f64.min,f64,f64,left,f64,right) { return floatMin(left,right); }
	DEFINE_INTRINSIC_FUNCTION2(wavmIntrinsics,f32Max,f32.max,f32,f32,left,f32,right) { return floatMax(left,right); }
	DEFINE_INTRINSIC_FUNCTION2(wavmIntrinsics,f64Max,f64.max,f64,f64,left,f64,right) { return floatMax(left,right); }

	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,f32Ceil,f32.ceil,f32,f32,value) { return floatCeil(value); }
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,f64Ceil,f64.ceil,f64,f64,value) { return floatCeil(value); }
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,f32Floor,f32.floor,f32,f32,value) { return floatFloor(value); }
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,f64Floor,f64.floor,f64,f64,value) { return floatFloor(value); }
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,f32Trunc,f32.trunc,f32,f32,value) { return floatTrunc(value); }
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,f64Trunc,f64.trunc,f64,f64,value) { return floatTrunc(value); }
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,f32Nearest,f32.nearest,f32,f32,value) { return floatNearest(value); }
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,f64Nearest,f64.nearest,f64,f64,value) { return floatNearest(value); }

	DEFINE_INTRINSIC_FUNCTION0(wavmIntrinsics,divideByZeroOrIntegerOverflowTrap,divideByZeroOrIntegerOverflowTrap,none)
	{
		throwException(Exception::integerDivideByZeroOrIntegerOverflowType);
	}

	DEFINE_INTRINSIC_FUNCTION0(wavmIntrinsics,unreachableTrap,unreachableTrap,none)
	{
		throwException(Exception::reachedUnreachableType);
	}
	
	DEFINE_INTRINSIC_FUNCTION0(wavmIntrinsics,accessViolationTrap,accessViolationTrap,none)
	{
		throwException(Exception::accessViolationType);
	}
	
	DEFINE_INTRINSIC_FUNCTION0(wavmIntrinsics,invalidFloatOperationTrap,invalidFloatOperationTrap,none)
	{
		throwException(Exception::invalidFloatOperationType);
	}
	
	DEFINE_INTRINSIC_FUNCTION3(wavmIntrinsics,indirectCallSignatureMismatch,indirectCallSignatureMismatch,none,i32,index,i64,expectedSignatureBits,i64,tableId)
	{
		TableInstance* table = getTableFromRuntimeData(contextRuntimeData,tableId);
		assert(table);
		void* elementValue = table->baseAddress[index].value;
		const FunctionType* actualSignature = table->baseAddress[index].type;
		const FunctionType* expectedSignature = reinterpret_cast<const FunctionType*>((Uptr)expectedSignatureBits);
		std::string ipDescription = "<unknown>";
		LLVMJIT::describeInstructionPointer(reinterpret_cast<Uptr>(elementValue),ipDescription);
		Log::printf(Log::Category::debug,"call_indirect signature mismatch: expected %s at index %u but got %s (%s)\n",
			asString(expectedSignature).c_str(),
			index,
			actualSignature ? asString(actualSignature).c_str() : "nullptr",
			ipDescription.c_str()
			);
		throwException(elementValue == nullptr ? Exception::undefinedTableElementType : Exception::indirectCallSignatureMismatchType);
	}

	DEFINE_INTRINSIC_FUNCTION0(wavmIntrinsics,indirectCallIndexOutOfBounds,indirectCallIndexOutOfBounds,none)
	{
		throwException(Exception::undefinedTableElementType);
	}

	DEFINE_INTRINSIC_FUNCTION2(wavmIntrinsics,_growMemory,growMemory,i32,i32,deltaPages,i64,memoryId)
	{
		MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,memoryId);
		assert(memory);
		const Iptr numPreviousMemoryPages = growMemory(memory,(Uptr)deltaPages);
		if(numPreviousMemoryPages + (Uptr)deltaPages > IR::maxMemoryPages) { return -1; }
		else { return (I32)numPreviousMemoryPages; }
	}
	
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,_currentMemory,currentMemory,i32,i64,memoryId)
	{
		MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,memoryId);
		assert(memory);
		Uptr numMemoryPages = getMemoryNumPages(memory);
		if(numMemoryPages > UINT32_MAX) { numMemoryPages = UINT32_MAX; }
		return (U32)numMemoryPages;
	}

	thread_local Uptr indentLevel = 0;

	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,debugEnterFunction,debugEnterFunction,none,i64,functionInstanceBits)
	{
		FunctionInstance* function = reinterpret_cast<FunctionInstance*>(functionInstanceBits);
		Log::printf(Log::Category::debug,"ENTER: %s\n",function->debugName.c_str());
		++indentLevel;
	}
	
	DEFINE_INTRINSIC_FUNCTION1(wavmIntrinsics,debugExitFunction,debugExitFunction,none,i64,functionInstanceBits)
	{
		FunctionInstance* function = reinterpret_cast<FunctionInstance*>(functionInstanceBits);
		--indentLevel;
		Log::printf(Log::Category::debug,"EXIT:  %s\n",function->debugName.c_str());
	}
	
	DEFINE_INTRINSIC_FUNCTION0(wavmIntrinsics,debugBreak,debugBreak,none)
	{
		Log::printf(Log::Category::debug,"================== wavmIntrinsics.debugBreak\n");
	}

	extern void dummyReferenceAtomics();

	Runtime::ModuleInstance* instantiateWAVMIntrinsics(Compartment* compartment)
	{
		dummyReferenceAtomics();
		return Intrinsics::instantiateModule(compartment,wavmIntrinsics);
	}
}
