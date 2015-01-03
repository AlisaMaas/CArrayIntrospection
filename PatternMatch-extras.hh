////////////////////////////////////////////////////////////////////////
//
//  Extra components for use with llvm::PatternMatch that could
//  (should?) have been included with LLVM in the first place.
//

#ifndef INCLUDE_PatternMatch_extras_hh
#define INCLUDE_PatternMatch_extras_hh

#include <llvm/Config/llvm-config.h>

#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 4)
#  include <llvm/IR/PatternMatch.h>
#else // LLVM 3.4 or earlier
#  include <llvm/Support/PatternMatch.h>
#endif // LLVM 3.4 or earlier


namespace llvm {
	namespace PatternMatch {
		// match a function formal argument
		inline bind_ty<Argument> m_FormalArgument(Argument *&argument) {
			return bind_ty<Argument>(argument);
		}


		template <typename Pointer>
		struct load_match {
			Pointer pointer;
			template <typename Mystery>
			bool match(Mystery *);
		};

		template <typename Pointer>
		template <typename Mystery>
		bool load_match<Pointer>::match(Mystery *mystery) {
			if (const auto load = dyn_cast<LoadInst>(mystery))
				return pointer.match(load->getPointerOperand());
			return false;
		}

		// match a load instruction
		template <typename Pointer>
		inline load_match<Pointer> m_Load(const Pointer &pointer) {
			return { pointer };
		}


		template <typename Pointer, typename Index>
		struct get_element_pointer_match {
			Pointer pointer;
			Index index;
			template <typename Mystery>
			bool match(Mystery *);
		};

		template <typename Pointer, typename Index>
		template <typename Mystery>
		bool get_element_pointer_match<Pointer, Index>::match(Mystery *mystery) {
			if (const auto gep = dyn_cast<GetElementPtrInst>(mystery)) {
				if (!pointer.match(gep->getPointerOperand()))
					return false;
				if (gep->getNumIndices() != 1)
					return false;
				if (!index.match(gep->idx_begin()))
					return false;
				return true;
			}
			return false;
		}

		// match a getelementptr instruction with exactly one index
		template <typename Pointer, typename Index>
		inline get_element_pointer_match<Pointer, Index> m_GetElementPointer(const Pointer &pointer, const Index &index) {
			return { pointer, index };
		}
	}
}


#endif // !INCLUDE_PatternMatch_extras_hh
