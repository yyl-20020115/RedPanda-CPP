/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "projectcompiler.h"
#include "../project.h"
#include "compilermanager.h"
#include "../systemconsts.h"
#include "qt_utils/charsetinfo.h"
#include "../editor.h"

#include <QDir>

ProjectCompiler::ProjectCompiler(std::shared_ptr<Project> project, bool silent, bool onlyCheckSyntax):
    Compiler("",silent,onlyCheckSyntax),
    mOnlyClean(false)
{
    setProject(project);
}

void ProjectCompiler::buildMakeFile()
{
    //we are using custom make file, don't overwrite it
    if (mProject->options().useCustomMakefile && !mProject->options().customMakefile.isEmpty())
        return;

    switch(mProject->options().type) {
    case ProjectType::StaticLib:
        createStaticMakeFile();
        break;
    case ProjectType::DynamicLib:
        createDynamicMakeFile();
        break;
    default:
        createStandardMakeFile();
    }
}

void ProjectCompiler::createStandardMakeFile()
{
    QFile file(mProject->makeFileName());
    newMakeFile(file);
    file.write("$(BIN): $(OBJ)\n");
    if (!mOnlyCheckSyntax) {
        if (mProject->options().isCpp) {
            writeln(file,"\t$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)");
        } else
            writeln(file,"\t$(CC) $(LINKOBJ) -o $(BIN) $(LIBS)");
    }
    writeMakeObjFilesRules(file);
}

void ProjectCompiler::createStaticMakeFile()
{
    QFile file(mProject->makeFileName());
    newMakeFile(file);
    writeln(file,"$(BIN): $(LINKOBJ)");
    if (!mOnlyCheckSyntax) {
      writeln(file,"\tar r $(BIN) $(LINKOBJ)");
      writeln(file,"\tranlib $(BIN)");
    }
    writeMakeObjFilesRules(file);
}

void ProjectCompiler::createDynamicMakeFile()
{
    QFile file(mProject->makeFileName());
    newMakeFile(file);
    writeln(file,"$(BIN): $(LINKOBJ)");
    if (!mOnlyCheckSyntax) {
        if (mProject->options().isCpp) {
          file.write("\t$(CPP) -mdll $(LINKOBJ) -o $(BIN) $(LIBS) -Wl,--output-def,$(DEF),--out-implib,$(STATIC)");
        } else {
          file.write("\t$(CC) -mdll $(LINKOBJ) -o $(BIN) $(LIBS) -Wl,--output-def,$(DEF),--out-implib,$(STATIC)");
        }
    }
    writeMakeObjFilesRules(file);
}

void ProjectCompiler::newMakeFile(QFile& file)
{
    // Create OBJ output directory
    if (!mProject->options().objectOutput.isEmpty()) {
        QDir(mProject->directory()).mkpath(mProject->options().objectOutput);
    }

    // Write more information to the log file than before
    log(tr("Building makefile..."));
    log("--------");
    log(tr("- Filename: %1").arg(mProject->makeFileName()));

    // Create the actual file
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw CompileError(tr("Can't open '%1' for write!").arg(mProject->makeFileName()));

    // Write header
    writeMakeHeader(file);

    // Writes definition list
    writeMakeDefines(file);

    // Write PHONY and all targets
    writeMakeTarget(file);

    // Write list of includes
    writeMakeIncludes(file);

    // Write clean command
    writeMakeClean(file);
}

void ProjectCompiler::writeMakeHeader(QFile &file)
{
    writeln(file,"# Project: " + mProject->name());
    writeln(file,QString("# Makefile created by Red Panda C++ ") + REDPANDA_CPP_VERSION);
    writeln(file);
    if (mOnlyCheckSyntax) {
        writeln(file,"# This Makefile is written for syntax check!");
        writeln(file,"# Regenerate it if you want to use this Makefile to build.");
        writeln(file);
    }
}

void ProjectCompiler::writeMakeDefines(QFile &file)
{
    // Get list of object files
    QString Objects;
    QString LinkObjects;
    QString cleanObjects;

    // Create a list of object files
    foreach(const PProjectUnit &unit, mProject->unitList()) {
        if (!unit->compile() && !unit->link())
            continue;

        // Only process source files
        QString RelativeName = extractRelativePath(mProject->directory(), unit->fileName());
        FileType fileType = getFileType(RelativeName);
        if (fileType == FileType::CSource || fileType == FileType::CppSource) {
            if (!mProject->options().objectOutput.isEmpty()) {
                // ofile = C:\MyProgram\obj\main.o
                QString fullObjFile = includeTrailingPathDelimiter(mProject->options().objectOutput)
                        + extractFileName(unit->fileName());
                QString relativeObjFile = extractRelativePath(mProject->directory(), changeFileExt(fullObjFile, OBJ_EXT));
                QString ObjFile = genMakePath2(relativeObjFile);
                Objects += ' ' + ObjFile;
#ifdef Q_OS_WIN
                cleanObjects += ' ' + genMakePath1(relativeObjFile).replace("/",QDir::separator());
#else
                cleanObjects += ' ' + genMakePath1(relativeObjFile);
#endif
                if (unit->link()) {
                    LinkObjects += ' ' + genMakePath1(relativeObjFile);
                }
            } else {
                Objects += ' ' + genMakePath2(changeFileExt(RelativeName, OBJ_EXT));
#ifdef Q_OS_WIN
                cleanObjects += ' ' + genMakePath1(changeFileExt(RelativeName, OBJ_EXT)).replace("/",QDir::separator());
#else
                cleanObjects += ' ' + genMakePath1(changeFileExt(RelativeName, OBJ_EXT));
#endif
                if (unit->link())
                    LinkObjects = LinkObjects + ' ' + genMakePath1(changeFileExt(RelativeName, OBJ_EXT));
            }
        }
    }

    Objects = Objects.trimmed();
    LinkObjects = LinkObjects.trimmed();

    // Get windres file
    QString ObjResFile;
#ifdef Q_OS_WIN
    if (!mProject->options().privateResource.isEmpty()) {
      if (!mProject->options().objectOutput.isEmpty()) {
          ObjResFile = includeTrailingPathDelimiter(mProject->options().objectOutput) +
                  changeFileExt(mProject->options().privateResource, RES_EXT);
      } else
          ObjResFile = changeFileExt(mProject->options().privateResource, RES_EXT);
    }
#endif

    // Mention progress in the logs
    if (!ObjResFile.isEmpty()) {
        log(tr("- Resource File: %1").arg(generateAbsolutePath(mProject->directory(),ObjResFile)));
    }
    log("");

    // Get list of applicable flags
    QString cCompileArguments = getCCompileArguments(mOnlyCheckSyntax);
    QString cppCompileArguments = getCppCompileArguments(mOnlyCheckSyntax);
    QString libraryArguments = getLibraryArguments(FileType::Project);
    QString cIncludeArguments = getCIncludeArguments() + " " + getProjectIncludeArguments();
    QString cppIncludeArguments = getCppIncludeArguments() + " " +getProjectIncludeArguments();

    if (cCompileArguments.indexOf(" -g3")>=0
            || cCompileArguments.startsWith("-g3")) {
        cCompileArguments += " -D__DEBUG__";
        cppCompileArguments+= " -D__DEBUG__";
    }

    writeln(file,"CPP      = " + extractFileName(compilerSet()->cppCompiler()));
    writeln(file,"CC       = " + extractFileName(compilerSet()->CCompiler()));
#ifdef Q_OS_WIN
    writeln(file,"WINDRES  = " + extractFileName(compilerSet()->resourceCompiler()));
#endif
    if (!ObjResFile.isEmpty()) {
      writeln(file,"RES      = " + genMakePath1(ObjResFile));
      writeln(file,"OBJ      = " + Objects + " $(RES)");
      writeln(file,"LINKOBJ  = " + LinkObjects + " $(RES)");
#ifdef Q_OS_WIN
      writeln(file,"CLEANOBJ  = " + cleanObjects +
              " " + genMakePath1(ObjResFile).replace("/",QDir::separator())
              + " " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())).replace("/",QDir::separator()) );
#else
      writeln(file,"CLEANOBJ  = " + cleanObjects +
              " " + genMakePath1(ObjResFile)
              + " " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())));
#endif
    } else {
      writeln(file,"OBJ      = " + Objects);
      writeln(file,"LINKOBJ  = " + LinkObjects);
#ifdef Q_OS_WIN
      writeln(file,"CLEANOBJ  = " + cleanObjects +
              + " " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())).replace("/",QDir::separator()) );
#else
      writeln(file,"CLEANOBJ  = " + cleanObjects +
              + " " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())));
#endif
    };
    libraryArguments.replace('\\', '/');
    writeln(file,"LIBS     = " + libraryArguments);
    cIncludeArguments.replace('\\', '/');
    writeln(file,"INCS     = " + cIncludeArguments);
    cppIncludeArguments.replace('\\', '/');
    writeln(file,"CXXINCS  = " + cppIncludeArguments);
    writeln(file,"BIN      = " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())));
    cppCompileArguments.replace('\\', '/');
    writeln(file,"CXXFLAGS = $(CXXINCS) " + cppCompileArguments);
    //writeln(file,"ENCODINGS = -finput-charset=utf-8 -fexec-charset='+GetSystemCharsetName);
    cCompileArguments.replace('\\', '/');
    writeln(file,"CFLAGS   = $(INCS) " + cCompileArguments);
    writeln(file, QString("RM       = ") + CLEAN_PROGRAM );
    if (mProject->options().usePrecompiledHeader){
        writeln(file,"PCH_H = " + mProject->options().precompiledHeader );
        writeln(file,"PCH = " + changeFileExt(mProject->options().precompiledHeader, GCH_EXT));
   }


    // This needs to be put in before the clean command.
    if (mProject->options().type == ProjectType::DynamicLib) {
        QString OutputFileDir = extractFilePath(mProject->executable());
        QString libOutputFile = includeTrailingPathDelimiter(OutputFileDir) + "lib" + extractFileName(mProject->executable());
        if (QFileInfo(libOutputFile).absoluteFilePath()
                == mProject->directory())
            libOutputFile = extractFileName(libOutputFile);
        else
            libOutputFile = extractRelativePath(mProject->makeFileName(), libOutputFile);
        writeln(file,"DEF      = " + genMakePath1(changeFileExt(libOutputFile, DEF_EXT)));
        writeln(file,"STATIC   = " + genMakePath1(changeFileExt(libOutputFile, LIB_EXT)));
#ifdef Q_OS_WIN
        writeln(file,"CLEAN_DEF      = " + genMakePath1(changeFileExt(libOutputFile, DEF_EXT)).replace("/",QDir::separator()));
        writeln(file,"CLEAN_STATIC   = " + genMakePath1(changeFileExt(libOutputFile, LIB_EXT)).replace("/",QDir::separator()));
#else
        writeln(file,"CLEAN_DEF      = " + genMakePath1(changeFileExt(libOutputFile, DEF_EXT)));
        writeln(file,"CLEAN_STATIC   = " + genMakePath1(changeFileExt(libOutputFile, LIB_EXT)));
#endif
    }
    writeln(file);
}

void ProjectCompiler::writeMakeTarget(QFile &file)
{
    if (mOnlyCheckSyntax)
        writeln(file, ".PHONY: all all-before all-after clean clean-custom $(OBJ) $(BIN)");
    else
        writeln(file, ".PHONY: all all-before all-after clean clean-custom");
    writeln(file);
    writeln(file, "all: all-before $(BIN) all-after");
    writeln(file);
    if (mProject->options().usePrecompiledHeader) {
        writeln(file, "$(PCH) : $(PCH_H)");
        writeln(file, "  $(CPP) -x c++-header \"$(PCH_H)\" -o \"$(PCH)\" $(CXXFLAGS)");
        writeln(file);
    }
}

void ProjectCompiler::writeMakeIncludes(QFile &file)
{
    foreach(const QString& s, mProject->options().makeIncludes) {
        writeln(file, "include " + genMakePath1(s));
    }
    if (!mProject->options().makeIncludes.isEmpty()) {
        writeln(file);
    }
}

void ProjectCompiler::writeMakeClean(QFile &file)
{
    writeln(file, "clean: clean-custom");
    if (mProject->options().type == ProjectType::DynamicLib)
        writeln(file, QString("\t${RM} $(CLEANOBJ) $(CLEAN_DEF) $(CLEAN_STATIC) > %1 2>&1").arg(NULL_FILE));
    else
        writeln(file, QString("\t${RM} $(CLEANOBJ) > %1 2>&1").arg(NULL_FILE));
    writeln(file);
}

void ProjectCompiler::writeMakeObjFilesRules(QFile &file)
{
    PCppParser parser = mProject->cppParser();
    QString precompileStr;
    if (mProject->options().usePrecompiledHeader)
        precompileStr = " $(PCH) ";

    QList<PProjectUnit> projectUnits=mProject->unitList();
    foreach(const PProjectUnit &unit, projectUnits) {
        FileType fileType = getFileType(unit->fileName());
        // Only process source files
        if (fileType!=FileType::CSource && fileType!=FileType::CppSource)
            continue;

        QString shortFileName = extractRelativePath(mProject->makeFileName(),unit->fileName());

        writeln(file);
        QString objStr=genMakePath2(shortFileName);
        // if we have scanned it, use scanned info
        if (parser && parser->scannedFiles().contains(unit->fileName())) {
            QSet<QString> fileIncludes = parser->getFileIncludes(unit->fileName());
            foreach (const QString& headerName, fileIncludes) {
                if (headerName == unit->fileName())
                    continue;
                if (!parser->isSystemHeaderFile(headerName)
                        && ! parser->isProjectHeaderFile(headerName)) {
                    foreach(const PProjectUnit &unit2, projectUnits) {
                        if (unit2->fileName()==headerName) {
                            objStr = objStr + ' ' + genMakePath2(extractRelativePath(mProject->makeFileName(),headerName));
                            break;
                        }
                    }
                }
            }
        } else {
            foreach(const PProjectUnit &unit2, projectUnits) {
                FileType fileType = getFileType(unit2->fileName());
                if (fileType == FileType::CHeader || fileType==FileType::CppHeader)
                    objStr = objStr + ' ' + genMakePath2(extractRelativePath(mProject->makeFileName(),unit2->fileName()));
            }
        }
        QString ObjFileName;
        QString ObjFileName2;
        if (!mProject->options().objectOutput.isEmpty()) {
            ObjFileName = includeTrailingPathDelimiter(mProject->options().objectOutput) +
                    extractFileName(unit->fileName());
            ObjFileName = genMakePath2(extractRelativePath(mProject->makeFileName(), changeFileExt(ObjFileName, OBJ_EXT)));
            ObjFileName2 = genMakePath1(extractRelativePath(mProject->makeFileName(), changeFileExt(ObjFileName, OBJ_EXT)));
//            if (!extractFileDir(ObjFileName).isEmpty()) {
//                objStr = genMakePath2(includeTrailingPathDelimiter(extractFileDir(ObjFileName))) + objStr;
//            }
        } else {
            ObjFileName = genMakePath2(changeFileExt(shortFileName, OBJ_EXT));
            ObjFileName2 = genMakePath1(changeFileExt(shortFileName, OBJ_EXT));
        }

        objStr = ObjFileName + ": "+objStr+precompileStr;

        writeln(file,objStr);

        // Write custom build command
        if (unit->overrideBuildCmd() && !unit->buildCmd().isEmpty()) {
            QString BuildCmd = unit->buildCmd();
            BuildCmd.replace("<CRTAB>", "\n\t");
            writeln(file, '\t' + BuildCmd);
            // Or roll our own
        } else {
            QString encodingStr;
            if (compilerSet()->compilerType() != CompilerType::Clang && mProject->options().addCharset) {
                QByteArray defaultSystemEncoding=pCharsetInfoManager->getDefaultSystemEncoding();
                QByteArray encoding = mProject->options().execEncoding;
                QByteArray targetEncoding;
                QByteArray sourceEncoding;
                if ( encoding == ENCODING_SYSTEM_DEFAULT || encoding.isEmpty()) {
                    targetEncoding = defaultSystemEncoding;
                } else if (encoding == ENCODING_UTF8_BOM) {
                    targetEncoding = "UTF-8";
                } else {
                    targetEncoding = encoding;
                }

                if (unit->encoding() == ENCODING_AUTO_DETECT) {
                    Editor* editor = mProject->unitEditor(unit);
                    if (editor && editor->fileEncoding()!=ENCODING_ASCII
                            && editor->fileEncoding()!=targetEncoding) {
                        sourceEncoding = editor->fileEncoding();
                    } else {
                        sourceEncoding = targetEncoding;
                    }
                } else if (unit->encoding()==ENCODING_SYSTEM_DEFAULT) {
                    sourceEncoding = defaultSystemEncoding;
                } else if (unit->encoding()!=ENCODING_ASCII && !unit->encoding().isEmpty()) {
                    sourceEncoding = unit->encoding();
                } else {
                    sourceEncoding = targetEncoding;
                }

                if (sourceEncoding!=targetEncoding) {
                    encodingStr = QString(" -finput-charset=%1 -fexec-charset=%2")
                            .arg(QString(sourceEncoding),
                                 QString(targetEncoding));
                }
            }

            if (mOnlyCheckSyntax) {
                if (unit->compileCpp())
                    writeln(file, "\t$(CPP) -c " + genMakePath1(shortFileName) + " $(CXXFLAGS) " + encodingStr);
                else
                    writeln(file, "\t(CC) -c " + genMakePath1(shortFileName) + " $(CFLAGS) " + encodingStr);
            } else {
                if (unit->compileCpp())
                    writeln(file, "\t$(CPP) -c " + genMakePath1(shortFileName) + " -o " + ObjFileName2 + " $(CXXFLAGS) " + encodingStr);
                else
                    writeln(file, "\t$(CC) -c " + genMakePath1(shortFileName) + " -o " + ObjFileName2 + " $(CFLAGS) " + encodingStr);
            }
        }
    }

#ifdef Q_OS_WIN
    if (!mProject->options().privateResource.isEmpty()) {

        // Concatenate all resource include directories
        QString ResIncludes(" ");
        for (int i=0;i<mProject->options().resourceIncludes.count();i++) {
            QString filename = mProject->options().resourceIncludes[i];
            if (!filename.isEmpty())
                ResIncludes = ResIncludes + " --include-dir " + genMakePath1(filename);
        }

        QString ResFiles;
        // Concatenate all resource filenames (not created when syntax checking)
        if (!mOnlyCheckSyntax) {
            foreach(const PProjectUnit& unit, mProject->unitList()) {
                if (getFileType(unit->fileName())!=FileType::WindowsResourceSource)
                    continue;
                if (fileExists(unit->fileName())) {
                    QString ResFile = extractRelativePath(mProject->makeFileName(), unit->fileName());
                    ResFiles = ResFiles + genMakePath2(ResFile) + ' ';
                }
            }
            ResFiles = ResFiles.trimmed();
        }

        // Determine resource output file
        QString ObjFileName;
        if (!mProject->options().objectOutput.isEmpty()) {
            ObjFileName = includeTrailingPathDelimiter(mProject->options().objectOutput) +
                  changeFileExt(mProject->options().privateResource, RES_EXT);
        } else {
            ObjFileName = changeFileExt(mProject->options().privateResource, RES_EXT);
        }
        ObjFileName = genMakePath1(extractRelativePath(mProject->filename(), ObjFileName));
        QString PrivResName = genMakePath1(extractRelativePath(mProject->filename(), mProject->options().privateResource));

        // Build final cmd
        QString WindresArgs;
        if (getCCompileArguments(mOnlyCheckSyntax).contains("-m32"))
              WindresArgs = " -F pe-i386";

        if (mOnlyCheckSyntax) {
            writeln(file);
            writeln(file, ObjFileName + ':');
            writeln(file, "\t$(WINDRES) -i " + PrivResName + WindresArgs + " --input-format=rc -o nul -O coff" + ResIncludes);
        } else {
            writeln(file);
            writeln(file, ObjFileName + ": " + PrivResName + ' ' + ResFiles);
            writeln(file, "\t$(WINDRES) -i " + PrivResName + WindresArgs + " --input-format=rc -o " + ObjFileName + " -O coff"
                + ResIncludes);
        }
        writeln(file);
    }
#endif
}

void ProjectCompiler::writeln(QFile &file, const QString &s)
{
    if (!s.isEmpty())
        file.write(s.toLocal8Bit());
    file.write("\n");
}

bool ProjectCompiler::onlyClean() const
{
    return mOnlyClean;
}

void ProjectCompiler::setOnlyClean(bool newOnlyClean)
{
    mOnlyClean = newOnlyClean;
}

bool ProjectCompiler::prepareForRebuild()
{
    //we use make argument to clean
    return true;
}

bool ProjectCompiler::prepareForCompile()
{
    if (!mProject)
        return false;
    //initProgressForm();
    log(tr("Compiling project changes..."));
    log("--------");
    log(tr("- Project Filename: %1").arg(mProject->filename()));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");

    buildMakeFile();

    mCompiler = compilerSet()->make();

    QString parallelParam;
    if (mProject->options().allowParallelBuilding) {
        if (mProject->options().parellelBuildingJobs==0) {
            parallelParam = " --jobs";
        } else {
            parallelParam = QString(" -j%1").arg(mProject->options().parellelBuildingJobs);
        }
    }

    if (mOnlyClean) {
        mArguments = QString(" %1 -f \"%2\" clean").arg(parallelParam,
                                                        extractRelativePath(
                                                            mProject->directory(),
                                                            mProject->makeFileName()));
    } else if (mRebuild) {
        mArguments = QString(" %1 -f \"%2\" clean all").arg(parallelParam,
                                                            extractRelativePath(
                                                            mProject->directory(),
                                                            mProject->makeFileName()));
    } else {
        mArguments = QString(" %1 -f \"%2\" all").arg(parallelParam,
                                                      extractRelativePath(
                                                      mProject->directory(),
                                                      mProject->makeFileName()));
    }
    mDirectory = mProject->directory();

    log(tr("Processing makefile:"));
    log("--------");
    log(tr("- makefile processer: %1").arg(mCompiler));
    log(tr("- Command: %1 %2").arg(extractFileName(mCompiler)).arg(mArguments));
    log("");

    return true;
}
