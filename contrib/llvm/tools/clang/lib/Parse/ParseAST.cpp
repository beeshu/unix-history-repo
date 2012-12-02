//===--- ParseAST.cpp - Provide the clang::ParseAST method ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the clang::ParseAST method.
//
//===----------------------------------------------------------------------===//

#include "clang/Parse/ParseAST.h"
#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/CodeCompleteConsumer.h"
#include "clang/Sema/SemaConsumer.h"
#include "clang/Sema/ExternalSemaSource.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExternalASTSource.h"
#include "clang/AST/Stmt.h"
#include "clang/Parse/Parser.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include <cstdio>

using namespace clang;

//===----------------------------------------------------------------------===//
// Public interface to the file
//===----------------------------------------------------------------------===//

/// ParseAST - Parse the entire file specified, notifying the ASTConsumer as
/// the file is parsed.  This inserts the parsed decls into the translation unit
/// held by Ctx.
///
void clang::ParseAST(Preprocessor &PP, ASTConsumer *Consumer,
                     ASTContext &Ctx, bool PrintStats,
                     TranslationUnitKind TUKind,
                     CodeCompleteConsumer *CompletionConsumer,
                     bool SkipFunctionBodies) {

  OwningPtr<Sema> S(new Sema(PP, Ctx, *Consumer,
                                   TUKind,
                                   CompletionConsumer));

  // Recover resources if we crash before exiting this method.
  llvm::CrashRecoveryContextCleanupRegistrar<Sema> CleanupSema(S.get());
  
  ParseAST(*S.get(), PrintStats, SkipFunctionBodies);
}

void clang::ParseAST(Sema &S, bool PrintStats, bool SkipFunctionBodies) {
  // Collect global stats on Decls/Stmts (until we have a module streamer).
  if (PrintStats) {
    Decl::EnableStatistics();
    Stmt::EnableStatistics();
  }

  // Also turn on collection of stats inside of the Sema object.
  bool OldCollectStats = PrintStats;
  std::swap(OldCollectStats, S.CollectStats);

  ASTConsumer *Consumer = &S.getASTConsumer();

  OwningPtr<Parser> ParseOP(new Parser(S.getPreprocessor(), S,
                                       SkipFunctionBodies));
  Parser &P = *ParseOP.get();

  PrettyStackTraceParserEntry CrashInfo(P);

  // Recover resources if we crash before exiting this method.
  llvm::CrashRecoveryContextCleanupRegistrar<Parser>
    CleanupParser(ParseOP.get());

  S.getPreprocessor().EnterMainSourceFile();
  P.Initialize();
  S.Initialize();

  // C11 6.9p1 says translation units must have at least one top-level
  // declaration. C++ doesn't have this restriction. We also don't want to
  // complain if we have a precompiled header, although technically if the PCH
  // is empty we should still emit the (pedantic) diagnostic.
  Parser::DeclGroupPtrTy ADecl;
  ExternalASTSource *External = S.getASTContext().getExternalSource();
  if (External)
    External->StartTranslationUnit(Consumer);

  if (P.ParseTopLevelDecl(ADecl)) {
    if (!External && !S.getLangOpts().CPlusPlus)
      P.Diag(diag::ext_empty_translation_unit);
  } else {
    do {
      // If we got a null return and something *was* parsed, ignore it.  This
      // is due to a top-level semicolon, an action override, or a parse error
      // skipping something.
      if (ADecl && !Consumer->HandleTopLevelDecl(ADecl.get()))
	return;
    } while (!P.ParseTopLevelDecl(ADecl));
  }

  // Process any TopLevelDecls generated by #pragma weak.
  for (SmallVector<Decl*,2>::iterator
       I = S.WeakTopLevelDecls().begin(),
       E = S.WeakTopLevelDecls().end(); I != E; ++I)
    Consumer->HandleTopLevelDecl(DeclGroupRef(*I));
  
  Consumer->HandleTranslationUnit(S.getASTContext());

  std::swap(OldCollectStats, S.CollectStats);
  if (PrintStats) {
    llvm::errs() << "\nSTATISTICS:\n";
    P.getActions().PrintStats();
    S.getASTContext().PrintStats();
    Decl::PrintStats();
    Stmt::PrintStats();
    Consumer->PrintStats();
  }
}
