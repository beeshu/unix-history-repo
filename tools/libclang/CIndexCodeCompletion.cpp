//===- CIndexCodeCompletion.cpp - Code Completion API hooks ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Clang-C Source Indexing library hooks for
// code completion.
//
//===----------------------------------------------------------------------===//

#include "CIndexer.h"
#include "CIndexDiagnostic.h"
#include "CLog.h"
#include "CXCursor.h"
#include "CXString.h"
#include "CXTranslationUnit.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/Type.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Sema/CodeCompleteConsumer.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>


#ifdef UDP_CODE_COMPLETION_LOGGER
#include "clang/Basic/Version.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace clang;
using namespace clang::cxindex;

extern "C" {

enum CXCompletionChunkKind
clang_getCompletionChunkKind(CXCompletionString completion_string,
                             unsigned chunk_number) {
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  if (!CCStr || chunk_number >= CCStr->size())
    return CXCompletionChunk_Text;

  switch ((*CCStr)[chunk_number].Kind) {
  case CodeCompletionString::CK_TypedText:
    return CXCompletionChunk_TypedText;
  case CodeCompletionString::CK_Text:
    return CXCompletionChunk_Text;
  case CodeCompletionString::CK_Optional:
    return CXCompletionChunk_Optional;
  case CodeCompletionString::CK_Placeholder:
    return CXCompletionChunk_Placeholder;
  case CodeCompletionString::CK_Informative:
    return CXCompletionChunk_Informative;
  case CodeCompletionString::CK_ResultType:
    return CXCompletionChunk_ResultType;
  case CodeCompletionString::CK_CurrentParameter:
    return CXCompletionChunk_CurrentParameter;
  case CodeCompletionString::CK_LeftParen:
    return CXCompletionChunk_LeftParen;
  case CodeCompletionString::CK_RightParen:
    return CXCompletionChunk_RightParen;
  case CodeCompletionString::CK_LeftBracket:
    return CXCompletionChunk_LeftBracket;
  case CodeCompletionString::CK_RightBracket:
    return CXCompletionChunk_RightBracket;
  case CodeCompletionString::CK_LeftBrace:
    return CXCompletionChunk_LeftBrace;
  case CodeCompletionString::CK_RightBrace:
    return CXCompletionChunk_RightBrace;
  case CodeCompletionString::CK_LeftAngle:
    return CXCompletionChunk_LeftAngle;
  case CodeCompletionString::CK_RightAngle:
    return CXCompletionChunk_RightAngle;
  case CodeCompletionString::CK_Comma:
    return CXCompletionChunk_Comma;
  case CodeCompletionString::CK_Colon:
    return CXCompletionChunk_Colon;
  case CodeCompletionString::CK_SemiColon:
    return CXCompletionChunk_SemiColon;
  case CodeCompletionString::CK_Equal:
    return CXCompletionChunk_Equal;
  case CodeCompletionString::CK_HorizontalSpace:
    return CXCompletionChunk_HorizontalSpace;
  case CodeCompletionString::CK_VerticalSpace:
    return CXCompletionChunk_VerticalSpace;
  }

  llvm_unreachable("Invalid CompletionKind!");
}

CXString clang_getCompletionChunkText(CXCompletionString completion_string,
                                      unsigned chunk_number) {
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  if (!CCStr || chunk_number >= CCStr->size())
    return cxstring::createNull();

  switch ((*CCStr)[chunk_number].Kind) {
  case CodeCompletionString::CK_TypedText:
  case CodeCompletionString::CK_Text:
  case CodeCompletionString::CK_Placeholder:
  case CodeCompletionString::CK_CurrentParameter:
  case CodeCompletionString::CK_Informative:
  case CodeCompletionString::CK_LeftParen:
  case CodeCompletionString::CK_RightParen:
  case CodeCompletionString::CK_LeftBracket:
  case CodeCompletionString::CK_RightBracket:
  case CodeCompletionString::CK_LeftBrace:
  case CodeCompletionString::CK_RightBrace:
  case CodeCompletionString::CK_LeftAngle:
  case CodeCompletionString::CK_RightAngle:
  case CodeCompletionString::CK_Comma:
  case CodeCompletionString::CK_ResultType:
  case CodeCompletionString::CK_Colon:
  case CodeCompletionString::CK_SemiColon:
  case CodeCompletionString::CK_Equal:
  case CodeCompletionString::CK_HorizontalSpace:
  case CodeCompletionString::CK_VerticalSpace:
    return cxstring::createRef((*CCStr)[chunk_number].Text);
      
  case CodeCompletionString::CK_Optional:
    // Note: treated as an empty text block.
    return cxstring::createEmpty();
  }

  llvm_unreachable("Invalid CodeCompletionString Kind!");
}


CXCompletionString
clang_getCompletionChunkCompletionString(CXCompletionString completion_string,
                                         unsigned chunk_number) {
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  if (!CCStr || chunk_number >= CCStr->size())
    return nullptr;

  switch ((*CCStr)[chunk_number].Kind) {
  case CodeCompletionString::CK_TypedText:
  case CodeCompletionString::CK_Text:
  case CodeCompletionString::CK_Placeholder:
  case CodeCompletionString::CK_CurrentParameter:
  case CodeCompletionString::CK_Informative:
  case CodeCompletionString::CK_LeftParen:
  case CodeCompletionString::CK_RightParen:
  case CodeCompletionString::CK_LeftBracket:
  case CodeCompletionString::CK_RightBracket:
  case CodeCompletionString::CK_LeftBrace:
  case CodeCompletionString::CK_RightBrace:
  case CodeCompletionString::CK_LeftAngle:
  case CodeCompletionString::CK_RightAngle:
  case CodeCompletionString::CK_Comma:
  case CodeCompletionString::CK_ResultType:
  case CodeCompletionString::CK_Colon:
  case CodeCompletionString::CK_SemiColon:
  case CodeCompletionString::CK_Equal:
  case CodeCompletionString::CK_HorizontalSpace:
  case CodeCompletionString::CK_VerticalSpace:
    return nullptr;

  case CodeCompletionString::CK_Optional:
    // Note: treated as an empty text block.
    return (*CCStr)[chunk_number].Optional;
  }

  llvm_unreachable("Invalid CompletionKind!");
}

unsigned clang_getNumCompletionChunks(CXCompletionString completion_string) {
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  return CCStr? CCStr->size() : 0;
}

unsigned clang_getCompletionPriority(CXCompletionString completion_string) {
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  return CCStr? CCStr->getPriority() : unsigned(CCP_Unlikely);
}
  
enum CXAvailabilityKind 
clang_getCompletionAvailability(CXCompletionString completion_string) {
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  return CCStr? static_cast<CXAvailabilityKind>(CCStr->getAvailability())
              : CXAvailability_Available;
}

unsigned clang_getCompletionNumAnnotations(CXCompletionString completion_string)
{
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  return CCStr ? CCStr->getAnnotationCount() : 0;
}

CXString clang_getCompletionAnnotation(CXCompletionString completion_string,
                                       unsigned annotation_number) {
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  return CCStr ? cxstring::createRef(CCStr->getAnnotation(annotation_number))
               : cxstring::createNull();
}

CXString
clang_getCompletionParent(CXCompletionString completion_string,
                          CXCursorKind *kind) {
  if (kind)
    *kind = CXCursor_NotImplemented;
  
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;
  if (!CCStr)
    return cxstring::createNull();
  
  return cxstring::createRef(CCStr->getParentContextName());
}

CXString
clang_getCompletionBriefComment(CXCompletionString completion_string) {
  CodeCompletionString *CCStr = (CodeCompletionString *)completion_string;

  if (!CCStr)
    return cxstring::createNull();

  return cxstring::createRef(CCStr->getBriefComment());
}

namespace {

/// \brief The CXCodeCompleteResults structure we allocate internally;
/// the client only sees the initial CXCodeCompleteResults structure.
///
/// Normally, clients of CXString shouldn't care whether or not a CXString is
/// managed by a pool or by explicitly malloc'ed memory.  But
/// AllocatedCXCodeCompleteResults outlives the CXTranslationUnit, so we can
/// not rely on the StringPool in the TU.
struct AllocatedCXCodeCompleteResults : public CXCodeCompleteResults {
  AllocatedCXCodeCompleteResults(IntrusiveRefCntPtr<FileManager> FileMgr);
  ~AllocatedCXCodeCompleteResults();
  
  /// \brief Diagnostics produced while performing code completion.
  SmallVector<StoredDiagnostic, 8> Diagnostics;

  /// \brief Allocated API-exposed wrappters for Diagnostics.
  SmallVector<CXStoredDiagnostic *, 8> DiagnosticsWrappers;

  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts;
  
  /// \brief Diag object
  IntrusiveRefCntPtr<DiagnosticsEngine> Diag;
  
  /// \brief Language options used to adjust source locations.
  LangOptions LangOpts;

  /// \brief File manager, used for diagnostics.
  IntrusiveRefCntPtr<FileManager> FileMgr;

  /// \brief Source manager, used for diagnostics.
  IntrusiveRefCntPtr<SourceManager> SourceMgr;
  
  /// \brief Temporary files that should be removed once we have finished
  /// with the code-completion results.
  std::vector<std::string> TemporaryFiles;

  /// \brief Temporary buffers that will be deleted once we have finished with
  /// the code-completion results.
  SmallVector<const llvm::MemoryBuffer *, 1> TemporaryBuffers;
  
  /// \brief Allocator used to store globally cached code-completion results.
  IntrusiveRefCntPtr<clang::GlobalCodeCompletionAllocator>
    CachedCompletionAllocator;
  
  /// \brief Allocator used to store code completion results.
  IntrusiveRefCntPtr<clang::GlobalCodeCompletionAllocator>
    CodeCompletionAllocator;
  
  /// \brief Context under which completion occurred.
  enum clang::CodeCompletionContext::Kind ContextKind;
  
  /// \brief A bitfield representing the acceptable completions for the
  /// current context.
  unsigned long long Contexts;
  
  /// \brief The kind of the container for the current context for completions.
  enum CXCursorKind ContainerKind;

  /// \brief The USR of the container for the current context for completions.
  std::string ContainerUSR;

  /// \brief a boolean value indicating whether there is complete information
  /// about the container
  unsigned ContainerIsIncomplete;
  
  /// \brief A string containing the Objective-C selector entered thus far for a
  /// message send.
  std::string Selector;
};

} // end anonymous namespace

/// \brief Tracks the number of code-completion result objects that are 
/// currently active.
///
/// Used for debugging purposes only.
static std::atomic<unsigned> CodeCompletionResultObjects;
  
AllocatedCXCodeCompleteResults::AllocatedCXCodeCompleteResults(
    IntrusiveRefCntPtr<FileManager> FileMgr)
    : CXCodeCompleteResults(),
      DiagOpts(new DiagnosticOptions),
      Diag(new DiagnosticsEngine(
          IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs), &*DiagOpts)),
      FileMgr(FileMgr), SourceMgr(new SourceManager(*Diag, *FileMgr)),
      CodeCompletionAllocator(new clang::GlobalCodeCompletionAllocator),
      Contexts(CXCompletionContext_Unknown),
      ContainerKind(CXCursor_InvalidCode), ContainerIsIncomplete(1) {
  if (getenv("LIBCLANG_OBJTRACKING"))
    fprintf(stderr, "+++ %u completion results\n",
            ++CodeCompletionResultObjects);
}
  
AllocatedCXCodeCompleteResults::~AllocatedCXCodeCompleteResults() {
  llvm::DeleteContainerPointers(DiagnosticsWrappers);
  delete [] Results;

  for (unsigned I = 0, N = TemporaryFiles.size(); I != N; ++I)
    llvm::sys::fs::remove(TemporaryFiles[I]);
  for (unsigned I = 0, N = TemporaryBuffers.size(); I != N; ++I)
    delete TemporaryBuffers[I];

  if (getenv("LIBCLANG_OBJTRACKING"))
    fprintf(stderr, "--- %u completion results\n",
            --CodeCompletionResultObjects);
}
  
} // end extern "C"

static unsigned long long getContextsForContextKind(
                                          enum CodeCompletionContext::Kind kind, 
                                                    Sema &S) {
  unsigned long long contexts = 0;
  switch (kind) {
    case CodeCompletionContext::CCC_OtherWithMacros: {
      //We can allow macros here, but we don't know what else is permissible
      //So we'll say the only thing permissible are macros
      contexts = CXCompletionContext_MacroName;
      break;
    }
    case CodeCompletionContext::CCC_TopLevel:
    case CodeCompletionContext::CCC_ObjCIvarList:
    case CodeCompletionContext::CCC_ClassStructUnion:
    case CodeCompletionContext::CCC_Type: {
      contexts = CXCompletionContext_AnyType | 
                 CXCompletionContext_ObjCInterface;
      if (S.getLangOpts().CPlusPlus) {
        contexts |= CXCompletionContext_EnumTag |
                    CXCompletionContext_UnionTag |
                    CXCompletionContext_StructTag |
                    CXCompletionContext_ClassTag |
                    CXCompletionContext_NestedNameSpecifier;
      }
      break;
    }
    case CodeCompletionContext::CCC_Statement: {
      contexts = CXCompletionContext_AnyType |
                 CXCompletionContext_ObjCInterface |
                 CXCompletionContext_AnyValue;
      if (S.getLangOpts().CPlusPlus) {
        contexts |= CXCompletionContext_EnumTag |
                    CXCompletionContext_UnionTag |
                    CXCompletionContext_StructTag |
                    CXCompletionContext_ClassTag |
                    CXCompletionContext_NestedNameSpecifier;
      }
      break;
    }
    case CodeCompletionContext::CCC_Expression: {
      contexts = CXCompletionContext_AnyValue;
      if (S.getLangOpts().CPlusPlus) {
        contexts |= CXCompletionContext_AnyType |
                    CXCompletionContext_ObjCInterface |
                    CXCompletionContext_EnumTag |
                    CXCompletionContext_UnionTag |
                    CXCompletionContext_StructTag |
                    CXCompletionContext_ClassTag |
                    CXCompletionContext_NestedNameSpecifier;
      }
      break;
    }
    case CodeCompletionContext::CCC_ObjCMessageReceiver: {
      contexts = CXCompletionContext_ObjCObjectValue |
                 CXCompletionContext_ObjCSelectorValue |
                 CXCompletionContext_ObjCInterface;
      if (S.getLangOpts().CPlusPlus) {
        contexts |= CXCompletionContext_CXXClassTypeValue |
                    CXCompletionContext_AnyType |
                    CXCompletionContext_EnumTag |
                    CXCompletionContext_UnionTag |
                    CXCompletionContext_StructTag |
                    CXCompletionContext_ClassTag |
                    CXCompletionContext_NestedNameSpecifier;
      }
      break;
    }
    case CodeCompletionContext::CCC_DotMemberAccess: {
      contexts = CXCompletionContext_DotMemberAccess;
      break;
    }
    case CodeCompletionContext::CCC_ArrowMemberAccess: {
      contexts = CXCompletionContext_ArrowMemberAccess;
      break;
    }
    case CodeCompletionContext::CCC_ObjCPropertyAccess: {
      contexts = CXCompletionContext_ObjCPropertyAccess;
      break;
    }
    case CodeCompletionContext::CCC_EnumTag: {
      contexts = CXCompletionContext_EnumTag |
                 CXCompletionContext_NestedNameSpecifier;
      break;
    }
    case CodeCompletionContext::CCC_UnionTag: {
      contexts = CXCompletionContext_UnionTag |
                 CXCompletionContext_NestedNameSpecifier;
      break;
    }
    case CodeCompletionContext::CCC_ClassOrStructTag: {
      contexts = CXCompletionContext_StructTag |
                 CXCompletionContext_ClassTag |
                 CXCompletionContext_NestedNameSpecifier;
      break;
    }
    case CodeCompletionContext::CCC_ObjCProtocolName: {
      contexts = CXCompletionContext_ObjCProtocol;
      break;
    }
    case CodeCompletionContext::CCC_Namespace: {
      contexts = CXCompletionContext_Namespace;
      break;
    }
    case CodeCompletionContext::CCC_PotentiallyQualifiedName: {
      contexts = CXCompletionContext_NestedNameSpecifier;
      break;
    }
    case CodeCompletionContext::CCC_MacroNameUse: {
      contexts = CXCompletionContext_MacroName;
      break;
    }
    case CodeCompletionContext::CCC_NaturalLanguage: {
      contexts = CXCompletionContext_NaturalLanguage;
      break;
    }
    case CodeCompletionContext::CCC_SelectorName: {
      contexts = CXCompletionContext_ObjCSelectorName;
      break;
    }
    case CodeCompletionContext::CCC_ParenthesizedExpression: {
      contexts = CXCompletionContext_AnyType |
                 CXCompletionContext_ObjCInterface |
                 CXCompletionContext_AnyValue;
      if (S.getLangOpts().CPlusPlus) {
        contexts |= CXCompletionContext_EnumTag |
                    CXCompletionContext_UnionTag |
                    CXCompletionContext_StructTag |
                    CXCompletionContext_ClassTag |
                    CXCompletionContext_NestedNameSpecifier;
      }
      break;
    }
    case CodeCompletionContext::CCC_ObjCInstanceMessage: {
      contexts = CXCompletionContext_ObjCInstanceMessage;
      break;
    }
    case CodeCompletionContext::CCC_ObjCClassMessage: {
      contexts = CXCompletionContext_ObjCClassMessage;
      break;
    }
    case CodeCompletionContext::CCC_ObjCInterfaceName: {
      contexts = CXCompletionContext_ObjCInterface;
      break;
    }
    case CodeCompletionContext::CCC_ObjCCategoryName: {
      contexts = CXCompletionContext_ObjCCategory;
      break;
    }
    case CodeCompletionContext::CCC_Other:
    case CodeCompletionContext::CCC_ObjCInterface:
    case CodeCompletionContext::CCC_ObjCImplementation:
    case CodeCompletionContext::CCC_Name:
    case CodeCompletionContext::CCC_MacroName:
    case CodeCompletionContext::CCC_PreprocessorExpression:
    case CodeCompletionContext::CCC_PreprocessorDirective:
    case CodeCompletionContext::CCC_TypeQualifiers: {
      //Only Clang results should be accepted, so we'll set all of the other
      //context bits to 0 (i.e. the empty set)
      contexts = CXCompletionContext_Unexposed;
      break;
    }
    case CodeCompletionContext::CCC_Recovery: {
      //We don't know what the current context is, so we'll return unknown
      //This is the equivalent of setting all of the other context bits
      contexts = CXCompletionContext_Unknown;
      break;
    }
  }
  return contexts;
}

namespace {
  class CaptureCompletionResults : public CodeCompleteConsumer {
    AllocatedCXCodeCompleteResults &AllocatedResults;
    CodeCompletionTUInfo CCTUInfo;
    SmallVector<CXCompletionResult, 16> StoredResults;
    CXTranslationUnit *TU;
  public:
    CaptureCompletionResults(const CodeCompleteOptions &Opts,
                             AllocatedCXCodeCompleteResults &Results,
                             CXTranslationUnit *TranslationUnit)
      : CodeCompleteConsumer(Opts, false), 
        AllocatedResults(Results), CCTUInfo(Results.CodeCompletionAllocator),
        TU(TranslationUnit) { }
    ~CaptureCompletionResults() { Finish(); }

    void ProcessCodeCompleteResults(Sema &S, 
                                    CodeCompletionContext Context,
                                    CodeCompletionResult *Results,
                                    unsigned NumResults) override {
      StoredResults.reserve(StoredResults.size() + NumResults);
      for (unsigned I = 0; I != NumResults; ++I) {
        CodeCompletionString *StoredCompletion        
          = Results[I].CreateCodeCompletionString(S, getAllocator(),
                                                  getCodeCompletionTUInfo(),
                                                  includeBriefComments());
        
        CXCompletionResult R;
        R.CursorKind = Results[I].CursorKind;
        R.CompletionString = StoredCompletion;
        StoredResults.push_back(R);
      }
      
      enum CodeCompletionContext::Kind contextKind = Context.getKind();
      
      AllocatedResults.ContextKind = contextKind;
      AllocatedResults.Contexts = getContextsForContextKind(contextKind, S);
      
      AllocatedResults.Selector = "";
      ArrayRef<IdentifierInfo *> SelIdents = Context.getSelIdents();
      for (ArrayRef<IdentifierInfo *>::iterator I = SelIdents.begin(),
                                                E = SelIdents.end();
           I != E; ++I) {
        if (IdentifierInfo *selIdent = *I)
          AllocatedResults.Selector += selIdent->getName();
        AllocatedResults.Selector += ":";
      }
      
      QualType baseType = Context.getBaseType();
      NamedDecl *D = nullptr;

      if (!baseType.isNull()) {
        // Get the declaration for a class/struct/union/enum type
        if (const TagType *Tag = baseType->getAs<TagType>())
          D = Tag->getDecl();
        // Get the @interface declaration for a (possibly-qualified) Objective-C
        // object pointer type, e.g., NSString*
        else if (const ObjCObjectPointerType *ObjPtr = 
                 baseType->getAs<ObjCObjectPointerType>())
          D = ObjPtr->getInterfaceDecl();
        // Get the @interface declaration for an Objective-C object type
        else if (const ObjCObjectType *Obj = baseType->getAs<ObjCObjectType>())
          D = Obj->getInterface();
        // Get the class for a C++ injected-class-name
        else if (const InjectedClassNameType *Injected =
                 baseType->getAs<InjectedClassNameType>())
          D = Injected->getDecl();
      }

      if (D != nullptr) {
        CXCursor cursor = cxcursor::MakeCXCursor(D, *TU);

        AllocatedResults.ContainerKind = clang_getCursorKind(cursor);

        CXString CursorUSR = clang_getCursorUSR(cursor);
        AllocatedResults.ContainerUSR = clang_getCString(CursorUSR);
        clang_disposeString(CursorUSR);

        const Type *type = baseType.getTypePtrOrNull();
        if (type) {
          AllocatedResults.ContainerIsIncomplete = type->isIncompleteType();
        }
        else {
          AllocatedResults.ContainerIsIncomplete = 1;
        }
      }
      else {
        AllocatedResults.ContainerKind = CXCursor_InvalidCode;
        AllocatedResults.ContainerUSR.clear();
        AllocatedResults.ContainerIsIncomplete = 1;
      }
    }

    void ProcessOverloadCandidates(Sema &S, unsigned CurrentArg,
                                   OverloadCandidate *Candidates,
                                   unsigned NumCandidates) override {
      StoredResults.reserve(StoredResults.size() + NumCandidates);
      for (unsigned I = 0; I != NumCandidates; ++I) {
        CodeCompletionString *StoredCompletion
          = Candidates[I].CreateSignatureString(CurrentArg, S, getAllocator(),
                                                getCodeCompletionTUInfo());
        
        CXCompletionResult R;
        R.CursorKind = CXCursor_NotImplemented;
        R.CompletionString = StoredCompletion;
        StoredResults.push_back(R);
      }
    }

    CodeCompletionAllocator &getAllocator() override {
      return *AllocatedResults.CodeCompletionAllocator;
    }

    CodeCompletionTUInfo &getCodeCompletionTUInfo() override { return CCTUInfo;}

  private:
    void Finish() {
      AllocatedResults.Results = new CXCompletionResult [StoredResults.size()];
      AllocatedResults.NumResults = StoredResults.size();
      std::memcpy(AllocatedResults.Results, StoredResults.data(), 
                  StoredResults.size() * sizeof(CXCompletionResult));
      StoredResults.clear();
    }
  };
}

extern "C" {
struct CodeCompleteAtInfo {
  CXTranslationUnit TU;
  const char *complete_filename;
  unsigned complete_line;
  unsigned complete_column;
  ArrayRef<CXUnsavedFile> unsaved_files;
  unsigned options;
  CXCodeCompleteResults *result;
};
void clang_codeCompleteAt_Impl(void *UserData) {
  CodeCompleteAtInfo *CCAI = static_cast<CodeCompleteAtInfo*>(UserData);
  CXTranslationUnit TU = CCAI->TU;
  const char *complete_filename = CCAI->complete_filename;
  unsigned complete_line = CCAI->complete_line;
  unsigned complete_column = CCAI->complete_column;
  unsigned options = CCAI->options;
  bool IncludeBriefComments = options & CXCodeComplete_IncludeBriefComments;
  CCAI->result = nullptr;

#ifdef UDP_CODE_COMPLETION_LOGGER
#ifdef UDP_CODE_COMPLETION_LOGGER_PORT
  const llvm::TimeRecord &StartTime =  llvm::TimeRecord::getCurrentTime();
#endif
#endif

  bool EnableLogging = getenv("LIBCLANG_CODE_COMPLETION_LOGGING") != nullptr;

  if (cxtu::isNotUsableTU(TU)) {
    LOG_BAD_TU(TU);
    return;
  }

  ASTUnit *AST = cxtu::getASTUnit(TU);
  if (!AST)
    return;

  CIndexer *CXXIdx = TU->CIdx;
  if (CXXIdx->isOptEnabled(CXGlobalOpt_ThreadBackgroundPriorityForEditing))
    setThreadBackgroundPriority();

  ASTUnit::ConcurrencyCheck Check(*AST);

  // Perform the remapping of source files.
  SmallVector<ASTUnit::RemappedFile, 4> RemappedFiles;

  for (auto &UF : CCAI->unsaved_files) {
    llvm::MemoryBuffer *MB =
        llvm::MemoryBuffer::getMemBufferCopy(getContents(UF), UF.Filename);
    RemappedFiles.push_back(std::make_pair(UF.Filename, MB));
  }

  if (EnableLogging) {
    // FIXME: Add logging.
  }

  // Parse the resulting source file to find code-completion results.
  AllocatedCXCodeCompleteResults *Results = new AllocatedCXCodeCompleteResults(
      &AST->getFileManager());
  Results->Results = nullptr;
  Results->NumResults = 0;
  
  // Create a code-completion consumer to capture the results.
  CodeCompleteOptions Opts;
  Opts.IncludeBriefComments = IncludeBriefComments;
  CaptureCompletionResults Capture(Opts, *Results, &TU);

  // Perform completion.
  AST->CodeComplete(complete_filename, complete_line, complete_column,
                    RemappedFiles,
                    (options & CXCodeComplete_IncludeMacros),
                    (options & CXCodeComplete_IncludeCodePatterns),
                    IncludeBriefComments,
                    Capture,
                    *Results->Diag, Results->LangOpts, *Results->SourceMgr,
                    *Results->FileMgr, Results->Diagnostics,
                    Results->TemporaryBuffers);

  Results->DiagnosticsWrappers.resize(Results->Diagnostics.size());

  // Keep a reference to the allocator used for cached global completions, so
  // that we can be sure that the memory used by our code completion strings
  // doesn't get freed due to subsequent reparses (while the code completion
  // results are still active).
  Results->CachedCompletionAllocator = AST->getCachedCompletionAllocator();

  

#ifdef UDP_CODE_COMPLETION_LOGGER
#ifdef UDP_CODE_COMPLETION_LOGGER_PORT
  const llvm::TimeRecord &EndTime =  llvm::TimeRecord::getCurrentTime();
  SmallString<256> LogResult;
  llvm::raw_svector_ostream os(LogResult);

  // Figure out the language and whether or not it uses PCH.
  const char *lang = 0;
  bool usesPCH = false;

  for (std::vector<const char*>::iterator I = argv.begin(), E = argv.end();
       I != E; ++I) {
    if (*I == 0)
      continue;
    if (strcmp(*I, "-x") == 0) {
      if (I + 1 != E) {
        lang = *(++I);
        continue;
      }
    }
    else if (strcmp(*I, "-include") == 0) {
      if (I+1 != E) {
        const char *arg = *(++I);
        SmallString<512> pchName;
        {
          llvm::raw_svector_ostream os(pchName);
          os << arg << ".pth";
        }
        pchName.push_back('\0');
        struct stat stat_results;
        if (stat(pchName.str().c_str(), &stat_results) == 0)
          usesPCH = true;
        continue;
      }
    }
  }

  os << "{ ";
  os << "\"wall\": " << (EndTime.getWallTime() - StartTime.getWallTime());
  os << ", \"numRes\": " << Results->NumResults;
  os << ", \"diags\": " << Results->Diagnostics.size();
  os << ", \"pch\": " << (usesPCH ? "true" : "false");
  os << ", \"lang\": \"" << (lang ? lang : "<unknown>") << '"';
  const char *name = getlogin();
  os << ", \"user\": \"" << (name ? name : "unknown") << '"';
  os << ", \"clangVer\": \"" << getClangFullVersion() << '"';
  os << " }";

  StringRef res = os.str();
  if (res.size() > 0) {
    do {
      // Setup the UDP socket.
      struct sockaddr_in servaddr;
      bzero(&servaddr, sizeof(servaddr));
      servaddr.sin_family = AF_INET;
      servaddr.sin_port = htons(UDP_CODE_COMPLETION_LOGGER_PORT);
      if (inet_pton(AF_INET, UDP_CODE_COMPLETION_LOGGER,
                    &servaddr.sin_addr) <= 0)
        break;

      int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      if (sockfd < 0)
        break;

      sendto(sockfd, res.data(), res.size(), 0,
             (struct sockaddr *)&servaddr, sizeof(servaddr));
      close(sockfd);
    }
    while (false);
  }
#endif
#endif
  CCAI->result = Results;
}
CXCodeCompleteResults *clang_codeCompleteAt(CXTranslationUnit TU,
                                            const char *complete_filename,
                                            unsigned complete_line,
                                            unsigned complete_column,
                                            struct CXUnsavedFile *unsaved_files,
                                            unsigned num_unsaved_files,
                                            unsigned options) {
  LOG_FUNC_SECTION {
    *Log << TU << ' '
         << complete_filename << ':' << complete_line << ':' << complete_column;
  }

  if (num_unsaved_files && !unsaved_files)
    return nullptr;

  CodeCompleteAtInfo CCAI = {TU, complete_filename, complete_line,
    complete_column, llvm::makeArrayRef(unsaved_files, num_unsaved_files),
    options, nullptr};

  if (getenv("LIBCLANG_NOTHREADS")) {
    clang_codeCompleteAt_Impl(&CCAI);
    return CCAI.result;
  }

  llvm::CrashRecoveryContext CRC;

  if (!RunSafely(CRC, clang_codeCompleteAt_Impl, &CCAI)) {
    fprintf(stderr, "libclang: crash detected in code completion\n");
    cxtu::getASTUnit(TU)->setUnsafeToFree(true);
    return nullptr;
  } else if (getenv("LIBCLANG_RESOURCE_USAGE"))
    PrintLibclangResourceUsage(TU);

  return CCAI.result;
}

unsigned clang_defaultCodeCompleteOptions(void) {
  return CXCodeComplete_IncludeMacros;
}

void clang_disposeCodeCompleteResults(CXCodeCompleteResults *ResultsIn) {
  if (!ResultsIn)
    return;

  AllocatedCXCodeCompleteResults *Results
    = static_cast<AllocatedCXCodeCompleteResults*>(ResultsIn);
  delete Results;
}
  
unsigned 
clang_codeCompleteGetNumDiagnostics(CXCodeCompleteResults *ResultsIn) {
  AllocatedCXCodeCompleteResults *Results
    = static_cast<AllocatedCXCodeCompleteResults*>(ResultsIn);
  if (!Results)
    return 0;

  return Results->Diagnostics.size();
}

CXDiagnostic 
clang_codeCompleteGetDiagnostic(CXCodeCompleteResults *ResultsIn,
                                unsigned Index) {
  AllocatedCXCodeCompleteResults *Results
    = static_cast<AllocatedCXCodeCompleteResults*>(ResultsIn);
  if (!Results || Index >= Results->Diagnostics.size())
    return nullptr;

  CXStoredDiagnostic *Diag = Results->DiagnosticsWrappers[Index];
  if (!Diag)
    Results->DiagnosticsWrappers[Index] = Diag =
        new CXStoredDiagnostic(Results->Diagnostics[Index], Results->LangOpts);
  return Diag;
}

unsigned long long
clang_codeCompleteGetContexts(CXCodeCompleteResults *ResultsIn) {
  AllocatedCXCodeCompleteResults *Results
    = static_cast<AllocatedCXCodeCompleteResults*>(ResultsIn);
  if (!Results)
    return 0;
  
  return Results->Contexts;
}

enum CXCursorKind clang_codeCompleteGetContainerKind(
                                               CXCodeCompleteResults *ResultsIn,
                                                     unsigned *IsIncomplete) {
  AllocatedCXCodeCompleteResults *Results =
    static_cast<AllocatedCXCodeCompleteResults *>(ResultsIn);
  if (!Results)
    return CXCursor_InvalidCode;

  if (IsIncomplete != nullptr) {
    *IsIncomplete = Results->ContainerIsIncomplete;
  }
  
  return Results->ContainerKind;
}
  
CXString clang_codeCompleteGetContainerUSR(CXCodeCompleteResults *ResultsIn) {
  AllocatedCXCodeCompleteResults *Results =
    static_cast<AllocatedCXCodeCompleteResults *>(ResultsIn);
  if (!Results)
    return cxstring::createEmpty();

  return cxstring::createRef(Results->ContainerUSR.c_str());
}

  
CXString clang_codeCompleteGetObjCSelector(CXCodeCompleteResults *ResultsIn) {
  AllocatedCXCodeCompleteResults *Results =
    static_cast<AllocatedCXCodeCompleteResults *>(ResultsIn);
  if (!Results)
    return cxstring::createEmpty();
  
  return cxstring::createDup(Results->Selector);
}
  
} // end extern "C"

/// \brief Simple utility function that appends a \p New string to the given
/// \p Old string, using the \p Buffer for storage.
///
/// \param Old The string to which we are appending. This parameter will be
/// updated to reflect the complete string.
///
///
/// \param New The string to append to \p Old.
///
/// \param Buffer A buffer that stores the actual, concatenated string. It will
/// be used if the old string is already-non-empty.
static void AppendToString(StringRef &Old, StringRef New,
                           SmallString<256> &Buffer) {
  if (Old.empty()) {
    Old = New;
    return;
  }
  
  if (Buffer.empty())
    Buffer.append(Old.begin(), Old.end());
  Buffer.append(New.begin(), New.end());
  Old = Buffer.str();
}

/// \brief Get the typed-text blocks from the given code-completion string
/// and return them as a single string.
///
/// \param String The code-completion string whose typed-text blocks will be
/// concatenated.
///
/// \param Buffer A buffer used for storage of the completed name.
static StringRef GetTypedName(CodeCompletionString *String,
                                    SmallString<256> &Buffer) {
  StringRef Result;
  for (CodeCompletionString::iterator C = String->begin(), CEnd = String->end();
       C != CEnd; ++C) {
    if (C->Kind == CodeCompletionString::CK_TypedText)
      AppendToString(Result, C->Text, Buffer);
  }
  
  return Result;
}

namespace {
  struct OrderCompletionResults {
    bool operator()(const CXCompletionResult &XR, 
                    const CXCompletionResult &YR) const {
      CodeCompletionString *X
        = (CodeCompletionString *)XR.CompletionString;
      CodeCompletionString *Y
        = (CodeCompletionString *)YR.CompletionString;
      
      SmallString<256> XBuffer;
      StringRef XText = GetTypedName(X, XBuffer);
      SmallString<256> YBuffer;
      StringRef YText = GetTypedName(Y, YBuffer);
      
      if (XText.empty() || YText.empty())
        return !XText.empty();
            
      int result = XText.compare_lower(YText);
      if (result < 0)
        return true;
      if (result > 0)
        return false;
      
      result = XText.compare(YText);
      return result < 0;
    }
  };
}

extern "C" {
  void clang_sortCodeCompletionResults(CXCompletionResult *Results,
                                       unsigned NumResults) {
    std::stable_sort(Results, Results + NumResults, OrderCompletionResults());
  }
}
