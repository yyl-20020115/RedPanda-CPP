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
#include "cppparser.h"
#include "parserutils.h"
#include "../utils.h"
#include "qsynedit/highlighter/cpp.h"

#include <QApplication>
#include <QDate>
#include <QHash>
#include <QQueue>
#include <QThread>
#include <QTime>

static QAtomicInt cppParserCount(0);
CppParser::CppParser(QObject *parent) : QObject(parent),
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    mMutex()
#else
    mMutex(QMutex::Recursive)
#endif
{
    mParserId = cppParserCount.fetchAndAddRelaxed(1);
    mLanguage = ParserLanguage::CPlusPlus;
    mSerialCount = 0;
    updateSerialId();
    mUniqId = 0;
    mParsing = false;
    //mStatementList ; // owns the objects
    //mFilesToScan;
    //mIncludePaths;
    //mProjectIncludePaths;
    //mProjectFiles;
    // mCurrentScope;
    //mCurrentClassScope;
    //mSkipList;
    mParseLocalHeaders = true;
    mParseGlobalHeaders = true;
    mLockCount = 0;
    mIsSystemHeader = false;
    mIsHeader = false;
    mIsProjectFile = false;

    mCppKeywords = CppKeywords;
    mCppTypeKeywords = CppTypeKeywords;
    mEnabled = true;

    internalClear();

    //mNamespaces;
    //mBlockBeginSkips;
    //mBlockEndSkips;
    //mInlineNamespaceEndSkips;
}

CppParser::~CppParser()
{
    while (true) {
        //wait for all methods finishes running
        {
            QMutexLocker locker(&mMutex);
            if (!mParsing && (mLockCount == 0)) {
              mParsing = true;
              break;
            }
        }
        QThread::msleep(50);
        QCoreApplication* app = QApplication::instance();
        app->processEvents();
    }
    //qDebug()<<"-------- parser deleted ------------";
}

void CppParser::addHardDefineByLine(const QString &line)
{
    QMutexLocker  locker(&mMutex);
    if (line.startsWith('#')) {
        mPreprocessor.addHardDefineByLine(line.mid(1).trimmed());
    } else {
        mPreprocessor.addHardDefineByLine(line);
    }
}

void CppParser::addIncludePath(const QString &value)
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.addIncludePath(includeTrailingPathDelimiter(value));
}

void CppParser::removeProjectFile(const QString &value)
{
    QMutexLocker locker(&mMutex);

    mProjectFiles.remove(value);
    mFilesToScan.remove(value);
}

void CppParser::addProjectIncludePath(const QString &value)
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.addProjectIncludePath(includeTrailingPathDelimiter(value));
}

void CppParser::clearIncludePaths()
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.clearIncludePaths();
}

void CppParser::clearProjectIncludePaths()
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.clearProjectIncludePaths();
}

void CppParser::clearProjectFiles()
{
    QMutexLocker  locker(&mMutex);
    mProjectFiles.clear();
}

QList<PStatement> CppParser::getListOfFunctions(const QString &fileName, const QString &phrase, int line)
{
    QMutexLocker locker(&mMutex);
    QList<PStatement> result;
    if (mParsing)
        return result;

    PStatement statement = findStatementOf(fileName,phrase, line);
    if (!statement)
        return result;
    PStatement parentScope;
    if (statement->kind == StatementKind::skClass) {
        parentScope = statement;
    } else
        parentScope = statement->parentScope.lock();
    if (parentScope && parentScope->kind == StatementKind::skNamespace) {
        PStatementList namespaceStatementsList = findNamespace(parentScope->command);
        if (namespaceStatementsList) {
            for (PStatement& namespaceStatement  : *namespaceStatementsList) {
                result.append(
                            getListOfFunctions(fileName,line,statement,namespaceStatement));
            }
        }
    } else
        result.append(
                    getListOfFunctions(fileName,line,statement,parentScope)
                    );
    return result;
}

PStatement CppParser::findScopeStatement(const QString &filename, int line)
{
    QMutexLocker locker(&mMutex);
    if (mParsing) {
        return PStatement();
    }
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename);
    if (!fileIncludes)
        return PStatement();

    return fileIncludes->scopes.findScopeAtLine(line);
}

PFileIncludes CppParser::findFileIncludes(const QString &filename, bool deleteIt)
{
    QMutexLocker locker(&mMutex);
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename,PFileIncludes());
    if (deleteIt && fileIncludes)
        mPreprocessor.includesList().remove(filename);
    return fileIncludes;
}
QString CppParser::findFirstTemplateParamOf(const QString &fileName, const QString &phrase, const PStatement& currentScope)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return "";
    return doFindFirstTemplateParamOf(fileName,phrase,currentScope);
}

PStatement CppParser::findFunctionAt(const QString &fileName, int line)
{
    QMutexLocker locker(&mMutex);
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(fileName);
    if (!fileIncludes)
        return PStatement();
    for (PStatement& statement : fileIncludes->statements) {
        if (statement->kind != StatementKind::skFunction
                && statement->kind != StatementKind::skConstructor
                && statement->kind != StatementKind::skDestructor)
            continue;
        if (statement->line == line || statement->definitionLine == line)
            return statement;
    }
    return PStatement();
}

int CppParser::findLastOperator(const QString &phrase) const
{
    int i = phrase.length()-1;

    // Obtain stuff after first operator
    while (i>=0) {
        if ((i+1<phrase.length()) &&
                (phrase[i + 1] == '>') && (phrase[i] == '-'))
            return i;
        else if ((i+1<phrase.length()) &&
                 (phrase[i + 1] == ':') && (phrase[i] == ':'))
            return i;
        else if (phrase[i] == '.')
            return i;
        i--;
    }
    return -1;
}

PStatementList CppParser::findNamespace(const QString &name)
{
    QMutexLocker locker(&mMutex);
    return mNamespaces.value(name,PStatementList());
}

PStatement CppParser::findStatement(const QString &fullname)
{
    QMutexLocker locker(&mMutex);
    if (fullname.isEmpty())
        return PStatement();
    QStringList phrases = fullname.split("::");
    if (phrases.isEmpty())
        return PStatement();
    PStatement parentStatement;
    PStatement statement;
    foreach (const QString& phrase, phrases) {
        if (parentStatement && parentStatement->kind == StatementKind::skNamespace) {
            PStatementList lst = findNamespace(parentStatement->fullName);
            foreach (const PStatement& namespaceStatement, *lst) {
                statement = findMemberOfStatement(phrase,namespaceStatement);
                if (statement)
                    break;
            }
        } else {
            statement = findMemberOfStatement(phrase,parentStatement);
        }
        if (!statement)
            return PStatement();
        parentStatement = statement;
    }
    return statement;
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase, int line)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    return findStatementOf(fileName,phrase,findScopeStatement(fileName,line));
}

PStatement CppParser::findStatementOf(const QString &fileName,
                                      const QString &phrase,
                                      const PStatement& currentScope,
                                      PStatement &parentScopeType,
                                      bool force)
{
    QMutexLocker locker(&mMutex);
    PStatement result;
    parentScopeType = currentScope;
    if (mParsing && !force)
        return PStatement();

    //find the start scope statement
    QString namespaceName, remainder;
    QString nextScopeWord,operatorToken,memberName;
    PStatement statement;
    getFullNamespace(phrase, namespaceName, remainder);
    if (!namespaceName.isEmpty()) {  // (namespace )qualified Name
        PStatementList namespaceList = mNamespaces.value(namespaceName);

        if (!namespaceList || namespaceList->isEmpty())
            return PStatement();

        if (remainder.isEmpty())
            return namespaceList->front();

        remainder = splitPhrase(remainder,nextScopeWord,operatorToken,memberName);

        for (PStatement& currentNamespace: *namespaceList) {
            statement = findMemberOfStatement(nextScopeWord,currentNamespace);
            if (statement)
                break;
        }

        //not found in namespaces;
        if (!statement)
            return PStatement();
        // found in namespace
    } else if ((phrase.length()>2) &&
               (phrase[0]==':') && (phrase[1]==':')) {
        //global
        remainder= phrase.mid(2);
        remainder= splitPhrase(remainder,nextScopeWord,operatorToken,memberName);
        statement= findMemberOfStatement(nextScopeWord,PStatement());
        if (!statement)
            return PStatement();
    } else {
        //unqualified name
        parentScopeType = currentScope;
        remainder = splitPhrase(remainder,nextScopeWord,operatorToken,memberName);
        statement = findStatementStartingFrom(fileName,nextScopeWord,parentScopeType);
        if (!statement)
            return PStatement();
    }
    parentScopeType = currentScope;

    if (!memberName.isEmpty() && (statement->kind == StatementKind::skTypedef)) {
        PStatement typeStatement = findTypeDefinitionOf(fileName,statement->type, parentScopeType);
        if (typeStatement)
            statement = typeStatement;
    }

    //using alias like 'using std::vector;'
    if (statement->kind == StatementKind::skAlias) {
        statement = findAliasedStatement(statement);
        if (!statement)
            return PStatement();
    }

    if (statement->kind == StatementKind::skConstructor) {
        // we need the class, not the construtor
        statement = statement->parentScope.lock();
        if (!statement)
            return PStatement();
    }
    PStatement lastScopeStatement;
    QString typeName;
    PStatement typeStatement;
    while (!memberName.isEmpty()) {
        if (statement->kind!=StatementKind::skClass
                && operatorToken == "::") {
            return PStatement();
        }
        if (statement->kind == StatementKind::skVariable
                || statement->kind ==  StatementKind::skParameter
                || statement->kind ==  StatementKind::skFunction) {

            bool isSTLContainerFunctions = false;

            if (statement->kind == StatementKind::skFunction){
                PStatement parentScope = statement->parentScope.lock();
                if (parentScope
                    && STLContainers.contains(parentScope->fullName)
                    && STLElementMethods.contains(statement->command)
                    && lastScopeStatement) {
                    isSTLContainerFunctions = true;
                    PStatement lastScopeParent = lastScopeStatement->parentScope.lock();
                    typeName=findFirstTemplateParamOf(fileName,lastScopeStatement->type,
                                                      lastScopeParent );
                    typeStatement=findTypeDefinitionOf(fileName, typeName,
                                                       lastScopeParent );
                }
            }
            if (!isSTLContainerFunctions)
                typeStatement = findTypeDefinitionOf(fileName,statement->type, parentScopeType);

            //it's stl smart pointer
            if ((typeStatement)
                    && STLPointers.contains(typeStatement->fullName)
                    && (operatorToken == "->")) {
                PStatement parentScope = statement->parentScope.lock();
                typeName=findFirstTemplateParamOf(fileName,statement->type, parentScope);
                typeStatement=findTypeDefinitionOf(fileName, typeName,parentScope);
            } else if ((typeStatement)
                       && STLContainers.contains(typeStatement->fullName)
                       && nextScopeWord.endsWith(']')) {
                //it's a std container
                PStatement parentScope = statement->parentScope.lock();
                typeName = findFirstTemplateParamOf(fileName,statement->type,
                                                    parentScope);
                typeStatement = findTypeDefinitionOf(fileName, typeName,
                                                     parentScope);
            }
            lastScopeStatement = statement;
            if (typeStatement)
                statement = typeStatement;
        } else
            lastScopeStatement = statement;
        remainder = splitPhrase(remainder,nextScopeWord,operatorToken,memberName);
        PStatement memberStatement = findMemberOfStatement(nextScopeWord,statement);
        if (!memberStatement)
            return PStatement();

        parentScopeType=statement;
        statement = memberStatement;
        if (!memberName.isEmpty() && (statement->kind == StatementKind::skTypedef)) {
            PStatement typeStatement = findTypeDefinitionOf(fileName,statement->type, parentScopeType);
            if (typeStatement)
                statement = typeStatement;
        }
    }
    return statement;
}

PEvalStatement CppParser::evalExpression(
        const QString &fileName,
        const QStringList &phraseExpression,
        const PStatement &currentScope)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PEvalStatement();
//    qDebug()<<phraseExpression;
    int pos = 0;
    return doEvalExpression(fileName,
                            phraseExpression,
                            pos,
                            currentScope,
                            PEvalStatement(),
                            true);
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase, const PStatement& currentClass, bool force)
{
    PStatement statementParentType;
    return findStatementOf(fileName,phrase,currentClass,statementParentType,force);
}

PStatement CppParser::findStatementOf(const QString &fileName, const QStringList &expression, const PStatement &currentScope)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    QString memberOperator;
    QStringList memberExpression;
    QStringList ownerExpression = getOwnerExpressionAndMember(expression,memberOperator,memberExpression);
    if (memberExpression.isEmpty()) {
        return PStatement();
    }
    QString phrase = memberExpression[0];
    if (memberOperator.isEmpty()) {
        return findStatementStartingFrom(fileName,phrase,currentScope);
    } else if (ownerExpression.isEmpty()) {
        return findMemberOfStatement(phrase,PStatement());
    } else {
        int pos = 0;
        PEvalStatement ownerEvalStatement = doEvalExpression(fileName,
                                ownerExpression,
                                pos,
                                currentScope,
                                PEvalStatement(),
                                true);
        if (!ownerEvalStatement) {
            return PStatement();
        }
        if (ownerEvalStatement->effectiveTypeStatement &&
                ownerEvalStatement->effectiveTypeStatement->kind == StatementKind::skNamespace) {
            PStatementList lst = findNamespace(ownerEvalStatement->effectiveTypeStatement->fullName);
            foreach (const PStatement& namespaceStatement, *lst) {
                PStatement statement = findMemberOfStatement(phrase,namespaceStatement);
                if (statement)
                    return statement;
            }
            return PStatement();
        }
        return findMemberOfStatement(phrase, ownerEvalStatement->effectiveTypeStatement);
    }

}

PStatement CppParser::findStatementOf(const QString &fileName, const QStringList &expression, int line)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    return findStatementOf(fileName,expression,findScopeStatement(fileName,line));
}

PStatement CppParser::findAliasedStatement(const PStatement &statement)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    if (!statement)
        return PStatement();
    QString alias = statement->type;
    int pos = statement->type.lastIndexOf("::");
    if (pos<0)
        return PStatement();
    QString nsName=statement->type.mid(0,pos);
    QString name = statement->type.mid(pos+2);
    PStatementList namespaceStatements = findNamespace(nsName);
    if (!namespaceStatements)
        return PStatement();
    foreach (const PStatement& namespaceStatement, *namespaceStatements) {
        QList<PStatement> resultList = findMembersOfStatement(name,namespaceStatement);
        foreach(const PStatement& resultStatement,resultList) {
            if (resultStatement->kind != StatementKind::skAlias)
                return resultStatement;
        }
    }
    return PStatement();
}

PStatement CppParser::findStatementStartingFrom(const QString &fileName, const QString &phrase, const PStatement& startScope)
{
    PStatement scopeStatement = startScope;

    // repeat until reach global
    PStatement result;
    while (scopeStatement) {
        //search members of current scope
        result = findStatementInScope(phrase, scopeStatement);
        if (result)
            return result;
        // not found
        // search members of all usings (in current scope )
        foreach (const QString& namespaceName, scopeStatement->usingList) {
            result = findStatementInNamespace(phrase,namespaceName);
            if (result)
                return result;
        }
        scopeStatement = scopeStatement->parentScope.lock();
    }

    // Search all global members
    result = findMemberOfStatement(phrase,PStatement());
    if (result)
        return result;

    //Find in all global usings
    const QSet<QString>& fileUsings = getFileUsings(fileName);
    // add members of all fusings
    for (const QString& namespaceName:fileUsings) {
        result = findStatementInNamespace(phrase,namespaceName);
        if (result)
            return result;
    }
    return PStatement();
}

PStatement CppParser::findTypeDefinitionOf(const QString &fileName, const QString &aType, const PStatement& currentClass)
{
    QMutexLocker locker(&mMutex);

    if (mParsing)
        return PStatement();

    // Remove pointer stuff from type
    QString s = aType; // 'Type' is a keyword
    int position = s.length()-1;
    while ((position >= 0) && (s[position] == '*'
                               || s[position] == ' '
                               || s[position] == '&'))
        position--;
    if (position != s.length()-1)
        s.truncate(position+1);

    // Strip template stuff
    position = s.indexOf('<');
    if (position >= 0) {
        int endPos = getBracketEnd(s,position);
        s.remove(position,endPos-position+1);
    }

    // Use last word only (strip 'const', 'static', etc)
    position = s.lastIndexOf(' ');
    if (position >= 0)
        s = s.mid(position+1);

    PStatement scopeStatement = currentClass;

    PStatement statement = findStatementOf(fileName,s,currentClass);
    return getTypeDef(statement,fileName,aType);
}

PStatement CppParser::findTypeDef(const PStatement &statement, const QString &fileName)
{
    QMutexLocker locker(&mMutex);

    if (mParsing)
        return PStatement();
    return getTypeDef(statement, fileName, "");
}

bool CppParser::freeze()
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return false;
    mLockCount++;
    return true;
}

bool CppParser::freeze(const QString &serialId)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return false;
    if (mSerialId!=serialId)
        return false;
    mLockCount++;
    return true;
}

QStringList CppParser::getClassesList()
{
    QMutexLocker locker(&mMutex);

    QStringList list;
    return list;
    // fills List with a list of all the known classes
    QQueue<PStatement> queue;
    queue.enqueue(PStatement());
    while (!queue.isEmpty()) {
        PStatement statement = queue.dequeue();
        StatementMap statementMap = mStatementList.childrenStatements(statement);
        for (PStatement& child:statementMap) {
            if (child->kind == StatementKind::skClass)
                list.append(child->command);
            if (!child->children.isEmpty())
                queue.enqueue(child);
        }
    }
    return list;
}

QStringList CppParser::getFileDirectIncludes(const QString &filename)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return QStringList();
    if (filename.isEmpty())
        return QStringList();
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename,PFileIncludes());

    if (fileIncludes) {
        return fileIncludes->directIncludes;
    }
    return QStringList();

}

QSet<QString> CppParser::getFileIncludes(const QString &filename)
{
    QMutexLocker locker(&mMutex);
    QSet<QString> list;
    if (mParsing)
        return list;
    if (filename.isEmpty())
        return list;
    list.insert(filename);
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename,PFileIncludes());

    if (fileIncludes) {
        foreach (const QString& file, fileIncludes->includeFiles.keys()) {
            list.insert(file);
        }
    }
    return list;
}

QSet<QString> CppParser::getFileUsings(const QString &filename)
{
    QMutexLocker locker(&mMutex);
    QSet<QString> result;
    if (filename.isEmpty())
        return result;
    if (mParsing)
        return result;
    PFileIncludes fileIncludes= mPreprocessor.includesList().value(filename,PFileIncludes());
    if (fileIncludes) {
        foreach (const QString& usingName, fileIncludes->usings) {
            result.insert(usingName);
        }
        foreach (const QString& subFile,fileIncludes->includeFiles.keys()){
            PFileIncludes subIncludes = mPreprocessor.includesList().value(subFile,PFileIncludes());
            if (subIncludes) {
                foreach (const QString& usingName, subIncludes->usings) {
                    result.insert(usingName);
                }
            }
        }
    }
    return result;
}

QString CppParser::getHeaderFileName(const QString &relativeTo, const QString &headerName, bool fromNext)
{
    QMutexLocker locker(&mMutex);
    QString currentDir = includeTrailingPathDelimiter(extractFileDir(relativeTo));
    QStringList includes;
    QStringList projectIncludes;
    bool found=false;
    if (fromNext && mPreprocessor.includePaths().contains(currentDir)) {
        foreach(const QString& s, mPreprocessor.includePathList()) {
            if (found) {
                includes.append(s);
                continue;
            } else if (s == currentDir)
                found = true;
        }
        projectIncludes = mPreprocessor.projectIncludePathList();
    } else if (fromNext && mPreprocessor.projectIncludePaths().contains(currentDir)) {
        includes = mPreprocessor.includePathList();
        foreach(const QString& s, mPreprocessor.projectIncludePathList()) {
            if (found) {
                includes.append(s);
                continue;
            } else if (s == currentDir)
                found = true;
        }
    } else {
        includes = mPreprocessor.includePathList();
        projectIncludes = mPreprocessor.projectIncludePathList();
    }
    return ::getHeaderFilename(relativeTo, headerName, includes,
                             projectIncludes);
}

void CppParser::invalidateFile(const QString &fileName)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing || mLockCount>0)
            return;
        updateSerialId();
        mParsing = true;
    }
    QSet<QString> files = calculateFilesToBeReparsed(fileName);
    internalInvalidateFiles(files);
    mParsing = false;
}

bool CppParser::isIncludeLine(const QString &line)
{
    QString trimmedLine = line.trimmed();
    if ((trimmedLine.length() > 0)
            && trimmedLine.startsWith('#')) { // it's a preprocessor line
        if (trimmedLine.mid(1).trimmed().startsWith("include"))
            return true;
    }
    return false;
}

bool CppParser::isIncludeNextLine(const QString &line)
{
    QString trimmedLine = line.trimmed();
    if ((trimmedLine.length() > 0)
            && trimmedLine.startsWith('#')) { // it's a preprocessor line
        if (trimmedLine.mid(1).trimmed().startsWith("include_next"))
            return true;
    }
    return false;
}

bool CppParser::isProjectHeaderFile(const QString &fileName)
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName,mPreprocessor.projectIncludePaths());
}

bool CppParser::isSystemHeaderFile(const QString &fileName)
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName,mPreprocessor.includePaths());
}

void CppParser::parseFile(const QString &fileName, bool inProject, bool onlyIfNotParsed, bool updateView)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing || mLockCount>0)
            return;
        updateSerialId();
        mParsing = true;
        if (updateView)
            emit onBusy();
        emit onStartParsing();
    }
    {
        auto action = finally([&,this]{
            mParsing = false;

            if (updateView)
                emit onEndParsing(mFilesScannedCount,1);
            else
                emit onEndParsing(mFilesScannedCount,0);
        });
        QString fName = fileName;
        if (onlyIfNotParsed && mPreprocessor.scannedFiles().contains(fName))
            return;

        if (inProject) {
            QSet<QString> filesToReparsed = calculateFilesToBeReparsed(fileName);
            QStringList files = sortFilesByIncludeRelations(filesToReparsed);
            internalInvalidateFiles(filesToReparsed);

            mFilesToScanCount = files.count();
            mFilesScannedCount = 0;

            foreach (const QString& file,files) {
                mFilesScannedCount++;
                emit onProgress(file,mFilesToScanCount,mFilesScannedCount);
                if (!mPreprocessor.scannedFiles().contains(file)) {
                    internalParse(file);
                }
            }
        } else {
            internalInvalidateFile(fileName);
            mFilesToScanCount = 1;
            mFilesScannedCount = 0;

            mFilesScannedCount++;
            emit onProgress(fileName,mFilesToScanCount,mFilesScannedCount);
            internalParse(fileName);
        }

//        if (inProject)
//            mProjectFiles.insert(fileName);
//        else {
//            mProjectFiles.remove(fileName);
//        }

        // Parse from disk or stream

    }
}

void CppParser::parseFileList(bool updateView)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing || mLockCount>0)
            return;
        updateSerialId();
        mParsing = true;
        if (updateView)
            emit onBusy();
        emit onStartParsing();
    }
    {
        auto action = finally([&,this]{
            mParsing = false;
            if (updateView)
                emit onEndParsing(mFilesScannedCount,1);
            else
                emit onEndParsing(mFilesScannedCount,0);
        });
        // Support stopping of parsing when files closes unexpectedly
        mFilesScannedCount = 0;
        mFilesToScanCount = mFilesToScan.count();

        QStringList files = sortFilesByIncludeRelations(mFilesToScan);
        // parse header files in the first parse
        foreach (const QString& file, files) {
            mFilesScannedCount++;
            emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
            if (!mPreprocessor.scannedFiles().contains(file)) {
                internalParse(file);
            }
        }
        mFilesToScan.clear();
    }
}

void CppParser::parseHardDefines()
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return;
    int oldIsSystemHeader = mIsSystemHeader;
    mIsSystemHeader = true;
    mParsing=true;
    {
        auto action = finally([&,this]{
            mParsing = false;
            mIsSystemHeader=oldIsSystemHeader;
        });
        for (const PDefine& define:mPreprocessor.hardDefines()) {
            addStatement(
                        PStatement(), // defines don't belong to any scope
                        "",
                        "", // define has no type
                        define->name,
                        define->args,
                        "",
                        define->value,
                        -1,
                        StatementKind::skPreprocessor,
                        StatementScope::Global,
                        StatementClassScope::None,
                        StatementProperty::spHasDefinition);
        }
    }
}

bool CppParser::parsing() const
{
    return mParsing;
}

void CppParser::resetParser()
{
    while (true) {
        {
            QMutexLocker locker(&mMutex);
            if (!mParsing && mLockCount ==0) {
                mParsing = true;
                break;
            }
        }
        QThread::msleep(50);
        QCoreApplication* app = QApplication::instance();
        app->processEvents();
    }
    {
        auto action = finally([this]{
            mParsing = false;
        });
        emit  onBusy();
        mUniqId = 0;

        mParseLocalHeaders = true;
        mParseGlobalHeaders = true;
        mIsSystemHeader = false;
        mIsHeader = false;
        mIsProjectFile = false;
        mFilesScannedCount=0;
        mFilesToScanCount = 0;

        mCurrentScope.clear();
        mCurrentClassScope.clear();
        mStatementList.clear();

        mProjectFiles.clear();
        mBlockBeginSkips.clear(); //list of for/catch block begin token index;
        mBlockEndSkips.clear(); //list of for/catch block end token index;
        mInlineNamespaceEndSkips.clear(); // list for inline namespace end token index;
        mFilesToScan.clear(); // list of base files to scan
        mNamespaces.clear();  // namespace and the statements in its scope
        mInlineNamespaces.clear();

        mPreprocessor.clear();
        mTokenizer.clear();

    }
}

void CppParser::unFreeze()
{
    QMutexLocker locker(&mMutex);
    mLockCount--;
}

QSet<QString> CppParser::scannedFiles()
{
    return mPreprocessor.scannedFiles();
}

bool CppParser::isFileParsed(const QString &filename)
{
    return mPreprocessor.scannedFiles().contains(filename);
}

QString CppParser::getScopePrefix(const PStatement& statement){
    switch (statement->classScope) {
    case StatementClassScope::Public:
        return "public";
    case StatementClassScope::Private:
        return "private";
    case StatementClassScope::Protected:
        return "protected";
    default:
        return "";
    }
}

QString CppParser::prettyPrintStatement(const PStatement& statement, const QString& filename, int line)
{
    QString result;
    switch(statement->kind) {
    case StatementKind::skPreprocessor:
        if (statement->command == "__FILE__")
            result = '"'+filename+'"';
        else if (statement->command == "__LINE__")
            result = QString("\"%1\"").arg(line);
        else if (statement->command == "__DATE__")
            result = QString("\"%1\"").arg(QDate::currentDate().toString(Qt::ISODate));
        else if (statement->command == "__TIME__")
            result = QString("\"%1\"").arg(QTime::currentTime().toString(Qt::ISODate));
        else {
            QString hintText = "#define";
            if (statement->command != "")
                hintText += ' ' + statement->command;
            if (statement->args != "")
                hintText += ' ' + statement->args;
            if (statement->value != "")
                hintText += ' ' + statement->value;
            result = hintText;
        }
        break;
    case StatementKind::skEnumClassType:
        result = "enum class "+statement->command;
        break;
    case StatementKind::skEnumType:
        result = "enum "+statement->command;
        break;
    case StatementKind::skEnum:
        result = statement->type + "::" + statement->command;
        break;
    case StatementKind::skTypedef:
        result = "typedef "+statement->type+" "+statement->command;
        if (!statement->args.isEmpty())
            result += " "+statement->args;
        break;
    case StatementKind::skAlias:
        result = "using "+statement->type;
        break;
    case StatementKind::skFunction:
    case StatementKind::skVariable:
    case StatementKind::skParameter:
    case StatementKind::skClass:
        if (statement->scope!= StatementScope::Local)
            result = getScopePrefix(statement)+ ' '; // public
        result += statement->type + ' '; // void
        result += statement->fullName; // A::B::C::Bar
        result += statement->args; // (int a)
        break;
    case StatementKind::skNamespace:
        result = statement->fullName; // Bar
        break;
    case StatementKind::skConstructor:
        result = getScopePrefix(statement); // public
        result += QObject::tr("constructor") + ' '; // constructor
        result += statement->type + ' '; // void
        result += statement->fullName; // A::B::C::Bar
        result += statement->args; // (int a)
        break;
    case StatementKind::skDestructor:
        result = getScopePrefix(statement); // public
        result += QObject::tr("destructor") + ' '; // constructor
        result += statement->type + ' '; // void
        result += statement->fullName; // A::B::C::Bar
        result += statement->args; // (int a)
        break;
    default:
        break;
    }
    return result;
}

QString CppParser::getTemplateParam(const PStatement& statement,
                                    const QString& filename,
                                    const QString& phrase,
                                    int index,
                                    const PStatement& currentScope)
{
    if (!statement)
        return "";
    if (statement->kind != StatementKind::skTypedef)
        return "";
    if (statement->type == phrase) // prevent infinite loop
        return "";
    return doFindTemplateParamOf(filename,statement->type,index,currentScope);
}

int CppParser::getTemplateParamStart(const QString &s, int startAt, int index)
{
    int i = startAt+1;
    int count=0;
    while (count<index) {
        i = getTemplateParamEnd(s,i) + 2;
        count++;
    }
    return i;
}

int CppParser::getTemplateParamEnd(const QString &s, int startAt) {
    int i = startAt;
    int level = 1; // pretend we start on top of '<'
    while (i < s.length()) {
        switch (s[i].unicode()) {
        case '<':
            level++;
            break;
        case ',':
            if (level == 1) {
                return i;
            }
            break;
        case '>':
            level--;
            if (level==0)
                return i;
        }
        i++;
    }
    return startAt;
}

void CppParser::addProjectFile(const QString &fileName, bool needScan)
{
    QMutexLocker locker(&mMutex);
    //value.replace('/','\\'); // only accept full file names

    // Update project listing
        mProjectFiles.insert(fileName);

    // Only parse given file
    if (needScan && !mPreprocessor.scannedFiles().contains(fileName)) {
        mFilesToScan.insert(fileName);
    }
}

PStatement CppParser::addInheritedStatement(const PStatement& derived, const PStatement& inherit, StatementClassScope access)
{

    PStatement statement = addStatement(
                derived,
                inherit->fileName,
                inherit->type, // "Type" is already in use
                inherit->command,
                inherit->args,
                inherit->noNameArgs,
                inherit->value,
                inherit->line,
                inherit->kind,
                inherit->scope,
                access,
                inherit->properties & StatementProperty::spInherited);
    return statement;
}

PStatement CppParser::addChildStatement(const PStatement& parent, const QString &fileName,
                                        const QString &aType,
                                        const QString &command, const QString &args,
                                        const QString& noNameArgs,
                                        const QString &value, int line, StatementKind kind,
                                        const StatementScope& scope, const StatementClassScope& classScope,
                                        StatementProperties properties)
{
    return addStatement(
                parent,
                fileName,
                aType,
                command,
                args,
                noNameArgs,
                value,
                line,
                kind,
                scope,
                classScope,
                properties);
}

PStatement CppParser::addStatement(const PStatement& parent,
                                   const QString &fileName,
                                   const QString &aType,
                                   const QString &command,
                                   const QString &args,
                                   const QString &noNameArgs,
                                   const QString &value,
                                   int line, StatementKind kind,
                                   const StatementScope& scope,
                                   const StatementClassScope& classScope,
                                   StatementProperties properties)
{
    // Move '*', '&' to type rather than cmd (it's in the way for code-completion)
    QString newType = aType;
    QString newCommand = command;
    while (!newCommand.isEmpty() && (newCommand.front() == '*' || newCommand.front() == '&')) {
        newType += newCommand.front();
        newCommand.remove(0,1); // remove first
    }
//    if (newCommand.startsWith("::") && parent && kind!=StatementKind::skBlock ) {
//        qDebug()<<command<<fileName<<line<<kind<<parent->fullName;
//    }

    if (kind == StatementKind::skConstructor
            || kind == StatementKind::skFunction
            || kind == StatementKind::skDestructor
            || kind == StatementKind::skVariable
            ) {
        //find
        if (properties.testFlag(StatementProperty::spHasDefinition)) {
            PStatement oldStatement = findStatementInScope(newCommand,noNameArgs,kind,parent);
            if (oldStatement  && !oldStatement->hasDefinition()) {
                oldStatement->setHasDefinition(true);
                if (oldStatement->fileName!=fileName) {
                    PFileIncludes fileIncludes=mPreprocessor.includesList().value(fileName);
                    if (fileIncludes) {
                        fileIncludes->statements.insert(oldStatement->fullName,
                                                         oldStatement);
                    }
                }
                oldStatement->definitionLine = line;
                oldStatement->definitionFileName = fileName;
                return oldStatement;
            }
        }
    }
    PStatement result = std::make_shared<Statement>();
    result->parentScope = parent;
    result->type = newType;
    if (!newCommand.isEmpty())
        result->command = newCommand;
    else {
        mUniqId++;
        result->command = QString("__STATEMENT__%1").arg(mUniqId);
    }
    result->args = args;
    result->noNameArgs = noNameArgs;
    result->value = value;
    result->kind = kind;
    result->scope = scope;
    result->classScope = classScope;
    result->properties = properties;
    result->line = line;
    result->definitionLine = line;
    result->fileName = fileName;
    result->definitionFileName = fileName;
    if (!fileName.isEmpty()) {
        result->setInProject(mIsProjectFile);
        result->setInSystemHeader(mIsSystemHeader);
    } else {
        result->setInProject(false);
        result->setInSystemHeader(true);
    }
    //result->children;
    //result->friends;
    if (scope == StatementScope::Local)
        result->fullName =  newCommand;
    else
        result->fullName =  getFullStatementName(newCommand, parent);
    result->usageCount = -1;
    mStatementList.add(result);
    if (result->kind == StatementKind::skNamespace) {
        PStatementList namespaceList = mNamespaces.value(result->fullName,PStatementList());
        if (!namespaceList) {
            namespaceList=std::make_shared<StatementList>();
            mNamespaces.insert(result->fullName,namespaceList);
        }
        namespaceList->append(result);
    }

    if (result->kind!= StatementKind::skBlock) {
        PFileIncludes fileIncludes = mPreprocessor.includesList().value(fileName);
        if (fileIncludes) {
            fileIncludes->statements.insert(result->fullName,result);
        }
    }
    return result;
}

PStatement CppParser::addStatement(const PStatement &parent,
                                   const QString &fileName,
                                   const QString &aType,
                                   const QString &command,
                                   int argStart, int argEnd,
                                   const QString &value, int line,
                                   StatementKind kind, const StatementScope &scope,
                                   const StatementClassScope &classScope,
                                   StatementProperties properties)
{
    Q_ASSERT(mTokenizer[argStart]->text=='(');
    QString args;
    QString noNameArgs;

    int start=argStart+1;
    bool typeGetted = false;
    int braceLevel=0;
    QString word;
    for (int i=start;i<argEnd;i++) {
        QChar ch=mTokenizer[i]->text[0];
        if (this->isIdentChar(ch)) {
            QString spaces=(i>argStart)?" ":"";
            args+=spaces;
            word += mTokenizer[i]->text;
            if (!typeGetted) {
                noNameArgs+=spaces+word;
                if (mCppTypeKeywords.contains(word) || !isCppKeyword(word))
                    typeGetted = true;
            } else {
                if (isCppKeyword(word)) {
                    noNameArgs+=spaces+word;
                }
            }
            word="";
        } else if (this->isDigitChar(ch)) {
            args+=" ";
        } else {
            switch(ch.unicode()) {
            case ',':
                if (braceLevel==0)
                    typeGetted=false;
                break;
            case '{':
            case '[':
            case '<':
            case '(':
                braceLevel++;
                break;
            case '}':
            case ']':
            case '>':
            case ')':
                braceLevel--;
                break;
                //todo: * and & processing
            case '*':
            case '&':
                if (braceLevel==0)
                    word+=ch;
                break;
            }
            noNameArgs+= mTokenizer[i]->text;
        }
        args+=mTokenizer[i]->text;
    }

    args="("+args.trimmed()+")";
    noNameArgs="("+noNameArgs.trimmed()+")";
    return addStatement(
                parent,
                fileName,
                aType,
                command,
                args,
                noNameArgs,
                value,
                line,
                kind,
                scope,
                classScope,
                properties);
}

void CppParser::setInheritance(int index, const PStatement& classStatement, bool isStruct)
{
    // Clear it. Assume it is assigned
    StatementClassScope lastInheritScopeType = StatementClassScope::None;
    // Assemble a list of statements in text form we inherit from
    while (true) {
        StatementClassScope inheritScopeType = getClassScope(mTokenizer[index]->text);
        QString currentText = mTokenizer[index]->text;
        if (currentText=='(') {
            //skip to matching ')'
            index=mTokenizer[index]->matchIndex;
        } else if (inheritScopeType == StatementClassScope::None) {
            if (currentText.front()!=','
                    && currentText.front()!=':') {
                QString basename = currentText;
                //remove template staff
                if (basename.endsWith('>')) {
                    int pBegin = basename.indexOf('<');
                    if (pBegin>=0)
                        basename.truncate(pBegin);
                }
                // Find the corresponding PStatement
                PStatement statement = findStatementOf(mCurrentFile,basename,
                                                       classStatement->parentScope.lock(),true);
                if (statement && statement->kind == StatementKind::skClass) {
                    inheritClassStatement(classStatement,isStruct,statement,lastInheritScopeType);
                }
            }
        }
        index++;
        lastInheritScopeType = inheritScopeType;
        if (index >= mTokenizer.tokenCount())
            break;
        if (mTokenizer[index]->text.front() == '{'
                || mTokenizer[index]->text.front() == ';')
            break;
    }
}

bool CppParser::isCurrentScope(const QString &command)
{
    PStatement statement = getCurrentScope();
    if (!statement)
        return false;
    QString s = command;
    // remove template staff
    if (s.endsWith('>')) {
        int i= command.indexOf('<');
        if (i>=0) {
            s.truncate(i);
        }
    }
    QString s2 = statement->command;
    if (s2.endsWith('>')) {
        int i= s2.indexOf('<');
        if (i>=0) {
            s2.truncate(i);
        }
    }
    return (s2 == s);
}

void CppParser::addSoloScopeLevel(PStatement& statement, int line, bool shouldResetBlock)
{
    // Add class list

    PStatement parentScope;
    if (shouldResetBlock && statement && (statement->kind == StatementKind::skBlock)) {
        parentScope = statement->parentScope.lock();
        while (parentScope && (parentScope->kind == StatementKind::skBlock)) {
            parentScope = parentScope->parentScope.lock();
        }
        if (!parentScope)
            statement.reset();
    }

    if (mCurrentClassScope.count()>0) {
        mCurrentClassScope.back() = mClassScope;
    }

    mCurrentScope.append(statement);

    PFileIncludes fileIncludes = mPreprocessor.includesList().value(mCurrentFile);

    if (fileIncludes) {
        fileIncludes->scopes.addScope(line,statement);
    }

    // Set new scope
    if (!statement)
        mClassScope = StatementClassScope::None; // {}, namespace or class that doesn't exist
    else if (statement->kind == StatementKind::skNamespace)
        mClassScope = StatementClassScope::None;
    else if (statement->type == "class")
        mClassScope = StatementClassScope::Private; // classes are private by default
    else
        mClassScope = StatementClassScope::Public; // structs are public by default
    mCurrentClassScope.append(mClassScope);
#ifdef QT_DEBUG
//    if (mCurrentClassScope.count()==1)
//        qDebug()<<"++add scope"<<mCurrentFile<<line<<mCurrentClassScope.count();
#endif
}

void CppParser::removeScopeLevel(int line)
{
    // Remove class list
    if (mCurrentScope.isEmpty())
        return; // TODO: should be an exception
#ifdef QT_DEBUG
//    if (mCurrentClassScope.count()==1)
//        qDebug()<<"--remove scope"<<mCurrentFile<<line<<mCurrentClassScope.count();
#endif
    PStatement currentScope = getCurrentScope();
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(mCurrentFile);
    if (currentScope) {
        if (currentScope->kind == StatementKind::skBlock) {
            if (currentScope->children.isEmpty()) {
                // remove no children block
                if (fileIncludes) {
                    fileIncludes->scopes.removeLastScope();
                }
                mStatementList.deleteStatement(currentScope);
            } else {
                fileIncludes->statements.insert(currentScope->fullName,currentScope);
            }
        } else if (currentScope->kind == StatementKind::skClass) {
            mIndex=indexOfNextSemicolon(mIndex);
        }
    }
    mCurrentScope.pop_back();
    mCurrentClassScope.pop_back();

    // Set new scope
    currentScope = getCurrentScope();
  //  fileIncludes:=FindFileIncludes(fCurrentFile);
    if (fileIncludes && fileIncludes->scopes.lastScope()!=currentScope) {
        fileIncludes->scopes.addScope(line,currentScope);
    }

    if (!currentScope) {
        mClassScope = StatementClassScope::None;
    } else {
        mClassScope = mCurrentClassScope.back();
    }
}

void CppParser::internalClear()
{
    mCurrentScope.clear();
    mCurrentClassScope.clear();
    mIndex = 0;
    mClassScope = StatementClassScope::None;
    mBlockBeginSkips.clear();
    mBlockEndSkips.clear();
    mInlineNamespaceEndSkips.clear();
}

QStringList CppParser::sortFilesByIncludeRelations(const QSet<QString> &files)
{
    QStringList result;
    QSet<QString> saveScannedFiles;

    saveScannedFiles=mPreprocessor.scannedFiles();

    //rebuild file include relations
    foreach(const QString& file, files) {
        if (mPreprocessor.scannedFiles().contains(file))
            continue;
        //already removed in interalInvalidateFiles
        //mPreprocessor.removeScannedFile(file);
        QStringList buffer;
        if (mOnGetFileStream) {
            mOnGetFileStream(file,buffer);
        }
        //we only use local include relations
        mPreprocessor.setScanOptions(false, true);
        mPreprocessor.preprocess(file,buffer);
        mPreprocessor.clearTempResults();
    }

    QSet<QString> fileSet=files;
    while (!fileSet.isEmpty()) {
        bool found=false;
        foreach (const QString& file,fileSet) {
            PFileIncludes fileIncludes = mPreprocessor.includesList().value(file);
            bool hasInclude=false;
            foreach(const QString& inc,fileIncludes->includeFiles.keys()) {
                if (fileSet.contains(inc)) {
                    hasInclude=true;
                    break;
                }
            }
            if (!hasInclude) {
                result.push_front(file);
                fileSet.remove(file);
                found=true;
                break;
            }
        }
        if (!found) {
            foreach (const QString& file,fileSet) {
                result.push_front(file);
                fileSet.remove(file);
            }
        }
    }
    QSet<QString> newScannedFiles = mPreprocessor.scannedFiles();
    foreach(const QString& file, newScannedFiles) {
        if (!saveScannedFiles.contains(file))
            mPreprocessor.removeScannedFile(file);
    }
    return result;
}

bool CppParser::checkForKeyword(KeywordType& keywordType)
{
    keywordType = mCppKeywords.value(mTokenizer[mIndex]->text,KeywordType::NotKeyword);
    switch(keywordType) {
    case KeywordType::Catch:
    case KeywordType::For:
    case KeywordType::Public:
    case KeywordType::Private:
    case KeywordType::Enum:
    case KeywordType::Inline:
    case KeywordType::Namespace:
    case KeywordType::Typedef:
    case KeywordType::Using:
    case KeywordType::Friend:
    case KeywordType::Protected:
    case KeywordType::None:
    case KeywordType::NotKeyword:
    case KeywordType::DeclType:
        return false;
    default:
        return true;
    }
}

bool CppParser::checkForMethod(QString &sType, QString &sName, int &argStartIndex,
                               int &argEndIndex, bool &isStatic, bool &isFriend)
{
    PStatement scope = getCurrentScope();

    if (scope && !(scope->kind == StatementKind::skNamespace
                   || scope->kind == StatementKind::skClass)) { //don't care function declaration in the function's
        return false;
    }

    // Function template:
    // compiler directives (>= 0 words), added to type
    // type (>= 1 words)
    // name (1 word)
    // (argument list)
    // ; or {

    isStatic = false;
    isFriend = false;

    sType = ""; // should contain type "int"
    sName = ""; // should contain function name "foo::function"

    bool bTypeOK = false;
    bool bNameOK = false;
    bool bArgsOK = false;

    // Don't modify index
    int indexBackup = mIndex;

    // Gather data for the string parts
    while ((mIndex < mTokenizer.tokenCount()) && !isSeperator(mTokenizer[mIndex]->text[0])) {
        if ((mIndex + 1 < mTokenizer.tokenCount())
                && (mTokenizer[mIndex + 1]->text == '(')) { // and start of a function
            int indexAfter = mTokenizer[mIndex + 1]->matchIndex+1;

            //it's not a function define
            if (indexAfter>=mTokenizer.tokenCount())
                break;
            //it's not a function define;
            if (isInvalidFunctionArgsSuffixChar(mTokenizer[indexAfter]->text[0]))
                break;
            //it's not a function define
            if (mTokenizer[indexAfter]->text[0] == ';') {
                //function can only be defined in global/namespaces/classes
                PStatement currentScope=getCurrentScope();
                if (currentScope) {
                    //in namespace, it might be function or object initilization
                    if (currentScope->kind == StatementKind::skNamespace
                           && isNotFuncArgs(mIndex + 1)) {
                        break;
                    //not in class, it can't be a valid function definition
                    } else if (currentScope->kind != StatementKind::skClass)
                        break;
                    //variable can't be initialized in class definition, it must be a function
                } else if (isNotFuncArgs(mIndex + 1))
                    break;
            }
            sName = mTokenizer[mIndex]->text;
            argStartIndex = mIndex+1;
            argEndIndex = mTokenizer[mIndex + 1]->matchIndex;
            bTypeOK = !sType.isEmpty();
            bNameOK = !sName.isEmpty();
            bArgsOK = true;

            // Allow constructor/destructor too
            if (!bTypeOK) {
                // Check for constructor/destructor outside class body
                int delimPos = sName.lastIndexOf("::");
                if (delimPos >= 0) {
                    bTypeOK = true;
                    sType = sName.mid(0, delimPos);

                    // remove template staff
                    int pos1 = sType.indexOf('<');
                    if (pos1>=0) {
                        sType.truncate(pos1);
                        sName = sType+sName.mid(delimPos);
                    }
                }
            }

            // Are we inside a class body?
            if (!bTypeOK) {
                sType = mTokenizer[mIndex]->text;
                if (sType[0] == '~')
                    sType.remove(0,1);
                bTypeOK = isCurrentScope(sType); // constructor/destructor
            }
            mIndex = argEndIndex+1;
            break;
        } else {
            //if IsValidIdentifier(mTokenizer[mIndex]->text) then
            // Still walking through type
            QString s = mTokenizer[mIndex]->text;
            if (s == "static")
                isStatic = true;
            if (s == "friend")
                isFriend = true;
            if (!s.isEmpty() && !(s=="extern"))
                sType = sType + ' '+ s;
            bTypeOK = !sType.isEmpty();
        }
        mIndex++;
    }

    // Correct function, don't jump over
    if (bTypeOK && bNameOK && bArgsOK) {
        sType = sType.trimmed(); // should contain type "int"
        sName = sName.trimmed(); // should contain function name "foo::function"
        return true;
    } else {
        mIndex = indexBackup;
        return false;
    }
}

bool CppParser::checkForNamespace(KeywordType keywordType)
{
    return (keywordType==KeywordType::Namespace &&(mIndex < mTokenizer.tokenCount()-1))
            || (
                keywordType==KeywordType::Inline
                && (mIndex+1 < mTokenizer.tokenCount()-1)
                &&mTokenizer[mIndex+1]->text == "namespace"
            );
}

bool CppParser::checkForPreprocessor()
{
    return (mTokenizer[mIndex]->text.startsWith('#'));
}

//bool CppParser::checkForLambda()
//{
//    return (mIndex+1<mTokenizer.tokenCount()
//            && mTokenizer[mIndex]->text.startsWith('[')
//            && mTokenizer[mIndex+1]->text=='(');
//}

bool CppParser::checkForScope(KeywordType keywordType)
{
    return ( (keywordType == KeywordType::Public || keywordType == KeywordType::Protected
              || keywordType == KeywordType::Private)
            && mIndex+1 < mTokenizer.tokenCount()
            && mTokenizer[mIndex + 1]->text == ':'
                );
}

bool CppParser::checkForStructs(KeywordType keywordType)
{
    int dis = 0;
    if (keywordType == KeywordType::Friend
            || keywordType == KeywordType::Public
            || keywordType == KeywordType::Private)
        dis = 1;
    if (mIndex >= mTokenizer.tokenCount() - 2 - dis)
        return false;
    QString word = mTokenizer[mIndex+dis]->text;
    int keyLen = calcKeyLenForStruct(word);
    if (keyLen<0)
        return false;
    bool result = (word.length() == keyLen) || isSpaceChar(word[keyLen])
            || (word[keyLen] == '[');

    if (result) {
        if (mTokenizer[mIndex + 2+dis]->text[0] != ';') { // not: class something;
            int i = mIndex+dis +1;
            // the check for ']' was added because of this example:
            // struct option long_options[] = {
            //		{"debug", 1, 0, 'D'},
            //		{"info", 0, 0, 'i'},
            //    ...
            // };
            while (i < mTokenizer.tokenCount()) {
                QChar ch = mTokenizer[i]->text.back();
                if (ch=='{' || ch == ':')
                    break;
                switch(ch.unicode()) {
                case ';':
                case '{':
                case '}':
                case ',':
                case '(':
                case ')':
                case '[':
                case ']':
                case '=':
                case '*':
                case '&':
                case '%':
                case '+':
                case '-':
                case '~':
                    return false;
                }
                i++;
            }
        }
    }
    return result;
}

bool CppParser::checkForTypedefEnum()
{
    //we assume that typedef is the current index, so we check the next
    //should call CheckForTypedef first!!!
    return (mIndex+1 < mTokenizer.tokenCount() ) &&
            (mTokenizer[mIndex + 1]->text == "enum");
}

bool CppParser::checkForTypedefStruct()
{
    //we assume that typedef is the current index, so we check the next
    //should call CheckForTypedef first!!!
    if (mIndex+1 >= mTokenizer.tokenCount())
        return false;
    QString word = mTokenizer[mIndex + 1]->text;
    int keyLen = calcKeyLenForStruct(word);
    if (keyLen<0)
        return false;
    return (word.length() == keyLen) || isSpaceChar(word[keyLen]) || word[keyLen]=='[';
}

bool CppParser::checkForUsing(KeywordType keywordType)
{
    return keywordType==KeywordType::Using && (mIndex < mTokenizer.tokenCount()-1);

}

void CppParser::checkAndHandleMethodOrVar(KeywordType keywordType)
{
    if (mIndex+2>=mTokenizer.tokenCount()) {
        mIndex+=2; // left's finish;
        return;
    }
    QString currentText=mTokenizer[mIndex]->text;
    if (keywordType==KeywordType::DeclType) {
        if (mTokenizer[mIndex+1]->text=='(') {
            currentText="auto";
            mIndex=mTokenizer[mIndex+1]->matchIndex+1;
        } else {
            currentText=mTokenizer[mIndex+1]->text;
            mIndex+=2;
        }
    } else
        mIndex++;
    //next token must be */&/word/(/{
    if (mTokenizer[mIndex]->text=='(') {
        int indexAfterParentheis=mTokenizer[mIndex]->matchIndex+1;
        if (indexAfterParentheis>=mTokenizer.tokenCount()) {
            //error
            mIndex=indexAfterParentheis;
        } else if (mTokenizer[indexAfterParentheis]->text=='(') {
            // operator overloading like (operator int)
            if (mTokenizer[mIndex+1]->text=="operator") {
                mIndex=indexAfterParentheis;
                handleMethod(StatementKind::skFunction,"",
                             mergeArgs(mIndex+1,mTokenizer[mIndex]->matchIndex-1),
                             indexAfterParentheis,false,false);
            } else if (currentText.endsWith("::operator")) {
                mIndex=indexAfterParentheis;
                handleMethod(StatementKind::skFunction,"",
                             mergeArgs(mIndex+1,mTokenizer[mIndex]->matchIndex-1),
                             indexAfterParentheis,false,false);
            } else {
                //function pointer var
                handleVar(currentText,false,false);
            }
        } else {
            if (currentText.startsWith("operator")
                    && currentText.length()>8
                    && !isIdentChar(currentText[8])) {
                // operator overloading
                handleMethod(StatementKind::skFunction,"",
                             currentText,
                             mIndex,false,false);
                return;
            }
            //check for constructor like Foo::Foo()
            QString name;
            QString temp,temp2;
            QString parentName;
            if (splitLastMember(currentText,name,temp)) {
                //has '::'
                bool isDestructor=false;
                if (!splitLastMember(temp,parentName,temp2))
                    parentName=temp;
                if (name.startsWith('~'))
                    name=name.mid(1);
                if (removeTemplateParams(name)==removeTemplateParams(parentName)) {
                    handleMethod( (isDestructor?StatementKind::skDestructor:StatementKind::skConstructor),
                                 "",
                                 currentText,
                                 mIndex,false,false);
                    return;
                }
            }
            // check for constructor like:
            // class Foo {
            //   Foo();
            // };
            PStatement scope=getCurrentScope();
            if (scope && scope->kind==StatementKind::skClass
                    && removeTemplateParams(scope->command) == removeTemplateParams(currentText)) {
                handleMethod(StatementKind::skConstructor,"",
                             currentText,
                             mIndex,false,false);
                return;
            }

            // function call, skip it
            mIndex=moveToEndOfStatement(mIndex,true);
        }
    } else if (mTokenizer[mIndex]->text.startsWith('*')
               || mTokenizer[mIndex]->text.startsWith('&')
               || tokenIsIdentifier(mTokenizer[mIndex]->text)
                   ) {
        // it should be function/var

        bool isStatic = false;
        bool isFriend = false;
        bool isExtern = false;

        QString sType = currentText; // should contain type "int"
        QString sName = ""; // should contain function name "foo::function"



        // Gather data for the string parts
        while (mIndex+1 < mTokenizer.tokenCount()) {
            if (mTokenizer[mIndex + 1]->text == '(') {
                if (mTokenizer[mIndex+2]->text == '*') {
                    //foo(*blabla), it's a function pointer var
                    handleVar(sType,isExtern,isStatic);
                    return;
                }

                int indexAfter=mTokenizer[mIndex + 1]->matchIndex+1;
                if (indexAfter>=mTokenizer.tokenCount()) {
                    //error
                    mIndex=indexAfter;
                    return;
                }
                //if it's like: foo(...)(...)
                if (mTokenizer[indexAfter]->text=='(') {
                    if (mTokenizer[mIndex]->text=="operator") {
                        //operator()() , it's an operator overload for ()
                        handleMethod(StatementKind::skFunction,sType,
                                     "operator()",indexAfter,isStatic,false);
                        return;
                    }
                    if (mTokenizer[mIndex]->text.endsWith("::operator")) {
                        // operator overloading
                        handleMethod(StatementKind::skFunction,"",
                                     mTokenizer[mIndex]->text+"()",indexAfter,isStatic,false);
                        return;
                    }
                    //foo(...)(...), it's a function pointer var
                    handleVar(sType,isExtern,isStatic);
                    //Won't implement: ignore function decl like int (text)(int x) { };
                    return;
                }
                //it's not a function define
                if (mTokenizer[indexAfter]->text[0] == ',') {
                    // var decl with init
                    handleVar(sType,isExtern,isStatic);
                    return;
                }
                if (mTokenizer[indexAfter]->text[0] == ';') {
                    //function can only be defined in global/namespaces/classes
                    PStatement currentScope=getCurrentScope();
                    if (currentScope) {
                        //in namespace, it might be function or object initilization
                        if (currentScope->kind == StatementKind::skNamespace
                               && isNotFuncArgs(mIndex + 1)) {
                            // var decl with init
                            handleVar(sType,isExtern,isStatic);
                            return;
                        //not in class, it can't be a valid function definition
                        } else if (currentScope->kind != StatementKind::skClass) {
                            // var decl with init
                            handleVar(sType,isExtern,isStatic);
                            return;
                        }
                        //variable can't be initialized in class definition, it must be a function
                    } else if (isNotFuncArgs(mIndex + 1)){
                        // var decl with init
                        handleVar(sType,isExtern,isStatic);
                        return;
                    }
                }
                sName = mTokenizer[mIndex]->text;
                mIndex++;

                handleMethod(StatementKind::skFunction,sType,
                             sName,mIndex,isStatic,isFriend);

                return;
            } else if (
                       mTokenizer[mIndex + 1]->text == ','
                       ||mTokenizer[mIndex + 1]->text == ';'
                       ||mTokenizer[mIndex + 1]->text == ':'
                       ||mTokenizer[mIndex + 1]->text == '{'
                       || mTokenizer[mIndex + 1]->text == '=') {
                handleVar(sType,isExtern,isStatic);
                return;
            } else {
                QString s = mTokenizer[mIndex]->text;
                if (s == "static")
                    isStatic = true;
                else if (s == "friend")
                    isFriend = true;
                else if (s == "extern")
                    isExtern = true;
                if (!s.isEmpty() && !(s=="extern"))
                    sType = sType + ' '+ s;
                mIndex++;
            }
        }
    }
}

QString CppParser::doFindFirstTemplateParamOf(const QString &fileName, const QString &phrase, const PStatement &currentScope)
{
    return doFindTemplateParamOf(fileName,phrase,0,currentScope);
}

QString CppParser::doFindTemplateParamOf(const QString &fileName, const QString &phrase, int index, const PStatement &currentScope)
{
    // Remove pointer stuff from type
    QString s = phrase; // 'Type' is a keyword
    int i = s.indexOf('<');
    if (i>=0) {
        i=getTemplateParamStart(s,i,index);
        int t=getTemplateParamEnd(s,i);
        //qDebug()<<index<<s<<s.mid(i,t-i)<<i<<t;
        return s.mid(i,t-i);
    }
    int position = s.length()-1;
    while ((position >= 0) && (s[position] == '*'
                               || s[position] == ' '
                               || s[position] == '&'))
        position--;
    if (position != s.length()-1)
        s.truncate(position+1);

    PStatement scopeStatement = currentScope;

    PStatement statement = findStatementOf(fileName,s,currentScope);
    return getTemplateParam(statement,fileName, phrase,index, currentScope);
}

int CppParser::getCurrentBlockEndSkip()
{
    if (mBlockEndSkips.isEmpty())
        return mTokenizer.tokenCount()+1;
    return mBlockEndSkips.back();
}

int CppParser::getCurrentBlockBeginSkip()
{
    if (mBlockBeginSkips.isEmpty())
        return mTokenizer.tokenCount()+1;
    return mBlockBeginSkips.back();
}

int CppParser::getCurrentInlineNamespaceEndSkip()
{
    if (mInlineNamespaceEndSkips.isEmpty())
        return mTokenizer.tokenCount()+1;
    return mInlineNamespaceEndSkips.back();
}

PStatement CppParser::getCurrentScope()
{
    if (mCurrentScope.isEmpty()) {
        return PStatement();
    }
    return mCurrentScope.back();
}

void CppParser::getFullNamespace(const QString &phrase, QString &sNamespace, QString &member)
{
    sNamespace = "";
    member = phrase;
    int strLen = phrase.length();
    if (strLen==0)
        return;
    int lastI =-1;
    int i=0;
    while (i<strLen) {
        if ((i+1<strLen) && (phrase[i]==':') && (phrase[i+1]==':') ) {
            if (!mNamespaces.contains(sNamespace)) {
                break;
            } else {
                lastI = i;
            }
        }
        sNamespace += phrase[i];
        i++;
    }
    if (i>=strLen) {
        if (mNamespaces.contains(sNamespace)) {
            sNamespace = phrase;
            member = "";
            return;
        }
    }
    if (lastI >= 0) {
        sNamespace = phrase.mid(0,lastI);
        member = phrase.mid(lastI+2);
    } else {
        sNamespace = "";
        member = phrase;
    }
}

QString CppParser::getFullStatementName(const QString &command, const PStatement& parent)
{
    PStatement scopeStatement=parent;
    while (scopeStatement && !isNamedScope(scopeStatement->kind))
        scopeStatement = scopeStatement->parentScope.lock();
    if (scopeStatement)
        return scopeStatement->fullName + "::" + command;
    else
        return command;
}

PStatement CppParser::getIncompleteClass(const QString &command, const PStatement& parentScope)
{
    QString s=command;
    //remove template parameter
    int p = s.indexOf('<');
    if (p>=0) {
        s.truncate(p);
    }
    PStatement result = findStatementOf(mCurrentFile,s,parentScope,true);
    if (result && result->kind!=StatementKind::skClass)
        return PStatement();
    return result;
}

StatementScope CppParser::getScope()
{
    // Don't blindly trust levels. Namespaces and externs can have levels too
    PStatement currentScope = getCurrentScope();

    // Invalid class or namespace/extern
    if (!currentScope || (currentScope->kind == StatementKind::skNamespace))
        return StatementScope::Global;
    else if (currentScope->kind == StatementKind::skClass)
        return StatementScope::ClassLocal;
    else
        return StatementScope::Local;
}

QString CppParser::getStatementKey(const QString &sName, const QString &sType, const QString &sNoNameArgs)
{
    return sName + "--" + sType + "--" + sNoNameArgs;
}

PStatement CppParser::getTypeDef(const PStatement& statement,
                                 const QString& fileName, const QString& aType)
{
    if (!statement) {
        return PStatement();
    }
    if (statement->kind == StatementKind::skClass
            || statement->kind == StatementKind::skEnumType
            || statement->kind == StatementKind::skEnumClassType) {
        return statement;
    } else if (statement->kind == StatementKind::skTypedef) {
        if (statement->type == aType) // prevent infinite loop
            return statement;
        PStatement result = findTypeDefinitionOf(fileName,statement->type, statement->parentScope.lock());
        if (!result) // found end of typedef trail, return result
            return statement;
        return result;
    } else if (statement->kind == StatementKind::skAlias) {
        PStatement result = findAliasedStatement(statement);
        if (!result) // found end of typedef trail, return result
            return statement;
        return result;
    } else
        return PStatement();
}

void CppParser::handleCatchBlock()
{
    int startLine= mTokenizer[mIndex]->line;
    mIndex++; // skip for/catch;
    if (!((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.startsWith('('))))
        return;
    //skip params
    int i2=mTokenizer[mIndex]->matchIndex+1;
    if (i2>=mTokenizer.tokenCount())
        return;
    if (mTokenizer[i2]->text.startsWith('{')) {
        mBlockBeginSkips.append(i2);
        int i = indexOfMatchingBrace(i2);
//        if (i==i2) {
//            mBlockEndSkips.append(mTokenizer.tokenCount());
//        } else {
        mBlockEndSkips.append(i);
    } else {
        int i=indexOfNextSemicolon(i2);
        mBlockEndSkips.append(i);
    }
    // add a block
    PStatement block = addStatement(
                getCurrentScope(),
                mCurrentFile,
                "",
                "",
                "",
                "",
                "",
                startLine,
                StatementKind::skBlock,
                getScope(),
                mClassScope,
                StatementProperty::spHasDefinition);
    addSoloScopeLevel(block,startLine);
    scanMethodArgs(block,mIndex);
    mIndex=mTokenizer[mIndex]->matchIndex+1;
}

void CppParser::handleEnum(bool isTypedef)
{
    QString enumName = "";
    bool isEnumClass = false;
    int startLine = mTokenizer[mIndex]->line;
    mIndex++; //skip 'enum'

    if (mIndex < mTokenizer.tokenCount() && mTokenizer[mIndex]->text == "class") {
        //enum class
        isEnumClass = true;
        mIndex++; //skip class

    }
    bool isAdhocVar=false;
    int endIndex=-1;
    if ((mIndex< mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith('{')) { // enum {...} NAME
        // Skip to the closing brace
        int i = indexOfMatchingBrace(mIndex);
        // Have we found the name?
        if (i + 1 < mTokenizer.tokenCount()) {
            enumName = mTokenizer[i + 1]->text.trimmed();
            if (!isIdentifierOrPointer(enumName)) {
                //not a valid enum, skip to j
                mIndex=indexOfNextSemicolon(i+1)+1;
                return;
            }
            if (!isTypedef) {
                //it's an ad-hoc enum var define;
                if (isEnumClass) {
                    //Enum class can't add hoc, just skip to ;
                    mIndex=indexOfNextSemicolon(i+1)+1;
                    return;
                }
                enumName = "__enum__"+enumName+"__";
                isAdhocVar=true;
            }
        }
        endIndex=i+1;
    } else if (mIndex+1< mTokenizer.tokenCount() && mTokenizer[mIndex+1]->text.startsWith('{')){ // enum NAME {...};
        enumName = mTokenizer[mIndex]->text;
    } else {
        // enum NAME blahblah
        // it's an old c-style enum variable definition
        return;
    }

    // Add statement for enum name too
    PStatement enumStatement;
    if (isEnumClass) {
        enumStatement=addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    "enum class",
                    enumName,
                    "",
                    "",
                    "",
                    startLine,
                    StatementKind::skEnumClassType,
                    getScope(),
                    mClassScope,
                    StatementProperty::spHasDefinition);
    } else {
        enumStatement=addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    "enum",
                    enumName,
                    "",
                    "",
                    "",
                    startLine,
                    StatementKind::skEnumType,
                    getScope(),
                    mClassScope,
                    StatementProperty::spHasDefinition);
    }
    if (isAdhocVar) {
        //Ad-hoc var definition
        // Skip to the closing brace
        int i = indexOfMatchingBrace(mIndex)+1;
        QString typeSuffix="";
        while (i<mTokenizer.tokenCount()) {
            QString name=mTokenizer[i]->text;
            if (isIdentifierOrPointer(name)) {
                QString suffix;
                QString args;
                parseCommandTypeAndArgs(name,suffix,args);
                if (!name.isEmpty()) {
                    addStatement(
                                getCurrentScope(),
                                mCurrentFile,
                                enumName+suffix,
                                mTokenizer[i]->text,
                                args,
                                "",
                                "",
                                mTokenizer[i]->line,
                                StatementKind::skVariable,
                                getScope(),
                                mClassScope,
                                StatementProperty::spHasDefinition);
                }
            } else if (name!=',') {
                break;
            }
            i++;
        }
        endIndex=indexOfNextSemicolon(i);
    }


    // Skip opening brace
    mIndex++;

    // Call every member "enum NAME ITEMNAME"
    QString lastType("enum");
    if (!enumName.isEmpty())
        lastType += ' ' + enumName;
    QString cmd;
    QString args;
    if (mTokenizer[mIndex]->text!='}') {
        while ((mIndex < mTokenizer.tokenCount()) &&
                         !isblockChar(mTokenizer[mIndex]->text[0])) {
            if (!mTokenizer[mIndex]->text.startsWith(',')) {
                cmd = mTokenizer[mIndex]->text;
                args = "";
                if (isEnumClass) {
                    if (enumStatement) {
                        addStatement(
                          enumStatement,
                          mCurrentFile,
                          lastType,
                          cmd,
                          args,
                          "",
                          "",
                          mTokenizer[mIndex]->line,
                          StatementKind::skEnum,
                          getScope(),
                          mClassScope,
                          StatementProperty::spHasDefinition);
                    }
                } else {
                    if (enumStatement) {
                        addStatement(
                          enumStatement,
                          mCurrentFile,
                          lastType,
                          cmd,
                          args,
                          "",
                          "",
                          mTokenizer[mIndex]->line,
                          StatementKind::skEnum,
                          getScope(),
                          mClassScope,
                          StatementProperty::spHasDefinition);
                    }
                    addStatement(
                                getCurrentScope(),
                                mCurrentFile,
                                lastType,
                                cmd,
                                "",
                                "",
                                "",
                                mTokenizer[mIndex]->line,
                                StatementKind::skEnum,
                                getScope(),
                                mClassScope,
                                StatementProperty::spHasDefinition);
                }
            }
            mIndex ++ ;
        }
    }
    if (mIndex<endIndex)
        mIndex=endIndex;
    mIndex = indexOfNextSemicolon(mIndex)+1;
}

void CppParser::handleForBlock()
{
    int startLine = mTokenizer[mIndex]->line;
    mIndex++; // skip for/catch;
    if (!(mIndex < mTokenizer.tokenCount()))
        return;
    int i=indexOfNextSemicolon(mIndex);
    int i2 = i+1; //skip over ';' (tokenizer have change for(;;) to for(;)
    if (i2>=mTokenizer.tokenCount())
        return;
    if (mTokenizer[i2]->text.startsWith('{')) {
        mBlockBeginSkips.append(i2);
        i=indexOfMatchingBrace(i2);
// tokenizer will handle unbalanced braces, no need check here
//        if (i==i2)
//            mBlockEndSkips.append(mTokenizer.tokenCount());
//        else
        mBlockEndSkips.append(i);
    } else {
        i=indexOfNextSemicolon(i2);
        mBlockEndSkips.append(i);
    }
    // add a block
    PStatement block = addStatement(
                getCurrentScope(),
                mCurrentFile,
                "",
                "",
                "",
                "",
                "",
                startLine,
                StatementKind::skBlock,
                getScope(),
                mClassScope,
                StatementProperty::spHasDefinition);

    addSoloScopeLevel(block,startLine);
}

void CppParser::handleKeyword(KeywordType skipType)
{
    // Skip
    switch (skipType) {
    case KeywordType::SkipItself:
        // skip it;
        mIndex++;
        break;
    case KeywordType::SkipNextSemicolon:
        // Skip to ; and over it
        skipNextSemicolon(mIndex);
        break;
    case KeywordType::SkipNextColon:
        // Skip to : and over it
        mIndex = indexOfNextColon(mIndex)+1;
        break;
    case KeywordType::SkipNextParenthesis:
        // skip pass ()
        skipParenthesis(mIndex);
        break;
    case KeywordType::MoveToLeftBrace:
        // Skip to {
        mIndex = indexOfNextLeftBrace(mIndex);
        break;
    case KeywordType::MoveToRightBrace:
        // Skip pass {}
        mIndex = indexPassBraces(mIndex);
        break;
    default:
        break;
    }
}

void CppParser::handleLambda(int index, int endIndex)
{
    Q_ASSERT(mTokenizer[index]->text.startsWith('['));
    int startLine=mTokenizer[index]->line;
    int argStart=index+1;
    if (mTokenizer[argStart]->text!='(')
        return;
    int argEnd= mTokenizer[argStart]->matchIndex;
    //TODO: parse captures
    int bodyStart=indexOfNextLeftBrace(argEnd+1);
    if (bodyStart>=endIndex) {
        return;
    }
    int bodyEnd = mTokenizer[bodyStart]->matchIndex;
    if (bodyEnd>=endIndex) {
        return;
    }
    PStatement lambdaBlock = addStatement(
                getCurrentScope(),
                mCurrentFile,
                "",
                "",
                "",
                "",
                "",
                startLine,
                StatementKind::skBlock,
                StatementScope::Local,
                StatementClassScope::None,
                StatementProperty::spHasDefinition);
    scanMethodArgs(lambdaBlock,argStart);
    addSoloScopeLevel(lambdaBlock,mTokenizer[bodyStart]->line);
    int i=bodyStart+1; // start after '{';
    while (i+2<bodyEnd) {
        if (tokenIsTypeOrNonKeyword(mTokenizer[i]->text)
                && !mTokenizer[i]->text.endsWith('.')
                && !mTokenizer[i]->text.endsWith("->")
                && (mTokenizer[i+1]->text.startsWith('*')
                 || mTokenizer[i+1]->text.startsWith('&')
                 || tokenIsTypeOrNonKeyword(mTokenizer[i+1]->text)))
        {
            QString sType;
            QString sName;
            while (i+1<bodyEnd) {
                if (mTokenizer[i+1]->text==':'
                        || mTokenizer[i+1]->text=='('
                        || mTokenizer[i+1]->text=='='
                        || mTokenizer[i+1]->text==';'
                        || mTokenizer[i+1]->text==','
                        || mTokenizer[i+1]->text=='{'
                        )
                    break;
                else {
                    if (!sType.isEmpty())
                        sType+=' ';
                    sType+=mTokenizer[i]->text;
                }
                i++;
            }
            QString tempType;
            while (i<bodyEnd) {
                // Skip bit identifiers,
                // e.g.:
                // handle
                // unsigned short bAppReturnCode:8,reserved:6,fBusy:1,fAck:1
                // as
                // unsigned short bAppReturnCode,reserved,fBusy,fAck
                if (mTokenizer[i]->text.front() == ':') {
                    while ( (i < mTokenizer.tokenCount())
                            && !(
                                mTokenizer[i]->text==','
                                || mTokenizer[i]->text==';'
                                || mTokenizer[i]->text=='='
                                ))
                        i++;
                } else if (mTokenizer[i]->text==';') {
                    break;
                } else if (isWordChar(mTokenizer[i]->text[0])) {
                    QString cmd=mTokenizer[i]->text;
                    while (cmd.startsWith('*')) {
                        cmd=cmd.mid(1);
                    }
                    if (cmd=="const") {
                        tempType="const";
                    } else {
                        QString suffix;
                        QString args;
                        cmd=mTokenizer[i]->text;
                        parseCommandTypeAndArgs(cmd,suffix,args);
                        if (!cmd.isEmpty()) {
                            addChildStatement(
                                        lambdaBlock,
                                        mCurrentFile,
                                        (sType+' '+tempType+suffix).trimmed(),
                                        cmd,
                                        args,
                                        "",
                                        "",
                                        mTokenizer[mIndex]->line,
                                        StatementKind::skVariable,
                                        getScope(),
                                        mClassScope,
                                        StatementProperty::spHasDefinition); // TODO: not supported to pass list
                            tempType="";
                        }
                    }
                    i++;
                } else if (mTokenizer[i]->text=='(') {
                    i=mTokenizer[i]->matchIndex+1;
                } else if (mTokenizer[i]->text=='=') {
                    i = skipAssignment(i, mTokenizer.tokenCount());
                } else if (mTokenizer[i]->text=='{') {
                    tempType="";
                    i=mTokenizer[i]->matchIndex+1;
                } else {
                    tempType="";
                    i++;
                }

            }
        }
        i=moveToEndOfStatement(i, true, bodyEnd);
    }
    removeScopeLevel(mTokenizer[bodyEnd]->line);
}

void CppParser::handleMethod(StatementKind functionKind,const QString &sType, const QString &sName, int argStart, bool isStatic, bool isFriend)
{
    bool isValid = true;
    bool isDeclaration = false; // assume it's not a prototype
    int startLine = mTokenizer[mIndex]->line;
    int argEnd = mTokenizer[argStart]->matchIndex;

    if (mIndex >= mTokenizer.tokenCount()) // not finished define, just skip it;
        return;

    PStatement scopeStatement = getCurrentScope();

    //find start of the function body;
    bool foundColon=false;
    mIndex=argEnd+1;
    while ((mIndex < mTokenizer.tokenCount()) && !isblockChar(mTokenizer[mIndex]->text.front())) {
        if (mTokenizer[mIndex]->text=='(') {
            mIndex=mTokenizer[mIndex]->matchIndex+1;
        }else if (mTokenizer[mIndex]->text==':') {
            foundColon=true;
            break;
        } else
            mIndex++;
    }
    if (foundColon) {
        mIndex++;
        while ((mIndex < mTokenizer.tokenCount()) && !isblockChar(mTokenizer[mIndex]->text.front())) {
            if (isWordChar(mTokenizer[mIndex]->text[0])
                    && mIndex+1<mTokenizer.tokenCount()
                    && mTokenizer[mIndex+1]->text=='{') {
                //skip parent {}intializer
                mIndex=mTokenizer[mIndex+1]->matchIndex+1;
            } else if (mTokenizer[mIndex]->text=='(') {
                mIndex=mTokenizer[mIndex]->matchIndex+1;
            } else
                mIndex++;
        }
    }

    if (mIndex>=mTokenizer.tokenCount())
        return;

    // Check if this is a prototype
    if (mTokenizer[mIndex]->text.startsWith(';')
            || mTokenizer[mIndex]->text.startsWith('}')) {// prototype
        isDeclaration = true;
    }

    QString scopelessName;
    PStatement functionStatement;
    if (isFriend && isDeclaration && scopeStatement) {
        int delimPos = sName.indexOf("::");
        if (delimPos >= 0) {
            scopelessName = sName.mid(delimPos+2);
        } else
            scopelessName = sName;
        //TODO : we should check namespace
        scopeStatement->friends.insert(scopelessName);
    } else if (isValid) {
        // Use the class the function belongs to as the parent ID if the function is declared outside of the class body
        QString scopelessName;
        QString parentClassName;
        if (splitLastMember(sName,scopelessName,parentClassName)) {
            // Provide Bar instead of Foo::Bar
            scopeStatement = getIncompleteClass(parentClassName,getCurrentScope());
        } else
            scopelessName = sName;


        // For function definitions, the parent class is given. Only use that as a parent
        if (!isDeclaration) {
            functionStatement=addStatement(
                        scopeStatement,
                        mCurrentFile,
                        sType,
                        scopelessName,
                        argStart,
                        argEnd,
                        "",
                        //mTokenizer[mIndex - 1]^.Line,
                        startLine,
                        functionKind,
                        getScope(),
                        mClassScope,
                        StatementProperty::spHasDefinition
                        | (isStatic?StatementProperty::spStatic:StatementProperty::spNone));
            scanMethodArgs(functionStatement, argStart);
            // add variable this to the class function
            if (scopeStatement && scopeStatement->kind == StatementKind::skClass &&
                    !isStatic) {
                //add this to non-static class member function
                addStatement(
                            functionStatement,
                            mCurrentFile,
                            scopeStatement->command+"*",
                            "this",
                            "",
                            "",
                            "",
                            startLine,
                            StatementKind::skVariable,
                            StatementScope::Local,
                            StatementClassScope::None,
                            StatementProperty::spHasDefinition);
            }

            // add "__func__ variable"
            addStatement(
                        functionStatement,
                        mCurrentFile,
                        "static const char ",
                        "__func__",
                        "[]",
                        "",
                        "\""+scopelessName+"\"",
                        startLine+1,
                        StatementKind::skVariable,
                        StatementScope::Local,
                        StatementClassScope::None,
                        StatementProperty::spHasDefinition);

        } else {
            functionStatement = addStatement(
                        scopeStatement,
                        mCurrentFile,
                        sType,
                        scopelessName,
                        argStart,
                        argEnd,
                        "",
                        //mTokenizer[mIndex - 1]^.Line,
                        startLine,
                        functionKind,
                        getScope(),
                        mClassScope,
                        (isStatic?StatementProperty::spStatic:StatementProperty::spNone));
        }

    }

    if ((mIndex < mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith('{')) {
        addSoloScopeLevel(functionStatement,startLine);
        mIndex++; //skip '{'
    } else if ((mIndex < mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith(';')) {
        addSoloScopeLevel(functionStatement,startLine);
        if (mTokenizer[mIndex]->line != startLine)
            removeScopeLevel(mTokenizer[mIndex]->line+1);
        else
            removeScopeLevel(startLine+1);
        mIndex++;
    }
}

void CppParser::handleNamespace(KeywordType skipType)
{
    bool isInline=false;
    int startLine = mTokenizer[mIndex]->line;

    if (skipType==KeywordType::Inline) {
        isInline = true;
        mIndex++; //skip 'inline'
    }

    mIndex++; //skip 'namespace'

//    if (!tokenIsIdentifier(mTokenizer[mIndex]->text))
//        //wrong namespace define, stop handling
//        return;
    QString command = mTokenizer[mIndex]->text;

    QString fullName = getFullStatementName(command,getCurrentScope());
    if (isInline) {
        mInlineNamespaces.insert(fullName);
    } else if (mInlineNamespaces.contains(fullName)) {
        isInline = true;
    }
//    if (command.startsWith("__")) // hack for inline namespaces
//      isInline = true;
    mIndex++;
    if (mIndex>=mTokenizer.tokenCount())
        return;
    QString aliasName;
    if ((mIndex+2<mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text == '=')) {
        aliasName=mTokenizer[mIndex+1]->text;
        //namespace alias
        addStatement(
            getCurrentScope(),
            mCurrentFile,
            aliasName, // name of the alias namespace
            command, // command
            "", // args
            "", // noname args
            "", // values
            //mTokenizer[mIndex]^.Line,
            startLine,
            StatementKind::skNamespaceAlias,
            getScope(),
            mClassScope,
            StatementProperty::spHasDefinition);
        mIndex+=2; //skip ;
        return;
    } else if (isInline) {
        //inline namespace , just skip it
        // Skip to '{'
        while ((mIndex<mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text != '{'))
            mIndex++;
        int i =indexOfMatchingBrace(mIndex); //skip '}'
        if (i==mIndex)
            mInlineNamespaceEndSkips.append(mTokenizer.tokenCount());
        else
            mInlineNamespaceEndSkips.append(i);
        if (mIndex<mTokenizer.tokenCount())
            mIndex++; //skip '{'
    } else {
        PStatement namespaceStatement = addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    "", // type
                    command, // command
                    "", // args
                    "", // noname args
                    "", // values
                    startLine,
                    StatementKind::skNamespace,
                    getScope(),
                    mClassScope,
                    StatementProperty::spHasDefinition);

        // find next '{' or ';'
        mIndex = indexOfNextSemicolonOrLeftBrace(mIndex);
        if (mIndex<mTokenizer.tokenCount() && mTokenizer[mIndex]->text=='{')
            addSoloScopeLevel(namespaceStatement,startLine);
        //skip it
        mIndex++;
    }
}

void CppParser::handleOtherTypedefs()
{
    int startLine = mTokenizer[mIndex]->line;
    // Skip typedef word
    mIndex++;

    if (mIndex>=mTokenizer.tokenCount())
        return;

    if (mTokenizer[mIndex]->text == '('
            || mTokenizer[mIndex]->text == ','
            || mTokenizer[mIndex]->text == ';') { // error typedef
        //skip over next ;
        mIndex=indexOfNextSemicolon(mIndex)+1;
        return;
    }
    if ((mIndex+1<mTokenizer.tokenCount())
            && (mTokenizer[mIndex+1]->text == ';')) {
        //no old type, not valid
        mIndex+=2; //skip ;
        return;
    }

    QString oldType;
    // Walk up to first new word (before first comma or ;)
    while(true) {
        oldType += mTokenizer[mIndex]->text + ' ';
        mIndex++;
        if (mIndex+1>=mTokenizer.tokenCount()) {
            //not valid, just exit
            return;
        }
        if  (mTokenizer[mIndex]->text=='(') {
            break;
        }
        if (mTokenizer[mIndex + 1]->text.front() == ','
                  || mTokenizer[mIndex + 1]->text == ';')
            break;
        //typedef function pointer

    }
    oldType = oldType.trimmed();
    if (oldType.isEmpty()) {
        //skip over next ;
        mIndex=indexOfNextSemicolon(mIndex)+1;
        return;
    }
    QString newType;
    while(mIndex+1<mTokenizer.tokenCount()) {
        if (mTokenizer[mIndex]->text == ',' ) {
            mIndex++;
        } else if (mTokenizer[mIndex]->text == ';' ) {
            break;
        } else if (mTokenizer[mIndex]->text == '(') {
            int paramStart=mTokenizer[mIndex]->matchIndex+1;
            QString newType = mTokenizer[mIndex+1]->text;
            if (paramStart>=mTokenizer.tokenCount()
                    || mTokenizer[paramStart]->text!='(') {
                //not valid function pointer (no args)
                //skip over next ;
                mIndex=indexOfNextSemicolon(paramStart)+1;
                return;
            }
            if (newType.startsWith('*'))
                newType = newType.mid(1);
            if (!newType.isEmpty()) {
                addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        oldType,
                        newType,
                        mergeArgs(paramStart,mTokenizer[paramStart]->matchIndex),
                        "",
                        "",
                        startLine,
                        StatementKind::skTypedef,
                        getScope(),
                        mClassScope,
                        StatementProperty::spHasDefinition);
            }
            mIndex = mTokenizer[paramStart]->matchIndex+1;
        } else if (mTokenizer[mIndex+1]->text.front() ==','
                       || mTokenizer[mIndex+1]->text.front() ==';') {
                newType += mTokenizer[mIndex]->text;
                QString suffix;
                QString args;
                parseCommandTypeAndArgs(newType,suffix,args);

                addStatement(
                            getCurrentScope(),
                            mCurrentFile,
                            oldType+suffix,
                            newType,
                            args,
                            "",
                            "",
                            startLine,
                            StatementKind::skTypedef,
                            getScope(),
                            mClassScope,
                            StatementProperty::spHasDefinition);
                newType = "";
                mIndex++;
        } else {
            newType += mTokenizer[mIndex]->text;
            mIndex++;
        }
    }

    // Step over semicolon (saves one HandleStatement loop)    
    mIndex++;
}

void CppParser::handlePreprocessor()
{
    QString text = mTokenizer[mIndex]->text.mid(1).trimmed();
    if (text.startsWith("include")) { // start of new file
        // format: #include fullfilename:line
        // Strip keyword
        QString s = text.mid(QString("include").length());
        if (!s.startsWith(" ") && !s.startsWith("\t"))
            goto handlePreprocessorEnd;
        int delimPos = s.lastIndexOf(':');
        if (delimPos>=0) {
//            qDebug()<<mCurrentScope.size()<<mCurrentFile<<mTokenizer[mIndex]->line<<s.mid(0,delimPos).trimmed();
            mCurrentFile = s.mid(0,delimPos).trimmed();
            mIsSystemHeader = isSystemHeaderFile(mCurrentFile) || isProjectHeaderFile(mCurrentFile);
            mIsProjectFile = mProjectFiles.contains(mCurrentFile);
            mIsHeader = isHFile(mCurrentFile);

            // Mention progress to user if we enter a NEW file
            bool ok;
            int line = s.midRef(delimPos+1).toInt(&ok);
            if (line == 1) {
                mFilesScannedCount++;
                mFilesToScanCount++;
                emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
            }
        }
    } else if (text.startsWith("define")) {

      // format: #define A B, remove define keyword
      QString s = text.mid(QString("define").length());
      if (!s.startsWith(" ") && !s.startsWith("\t"))
          goto handlePreprocessorEnd;
      s = s.trimmed();
      // Ask the preprocessor to cut parts up
      QString name,args,value;
      mPreprocessor.getDefineParts(s,name,args,value);

      addStatement(
                  nullptr, // defines don't belong to any scope
                  mCurrentFile,
                  "", // define has no type
                  name,
                  args,
                  "",// noname args
                  value,
                  mTokenizer[mIndex]->line,
                  StatementKind::skPreprocessor,
                  StatementScope::Global,
                  StatementClassScope::None,
                  StatementProperty::spHasDefinition);
    } // TODO: undef ( define has limited scope)
handlePreprocessorEnd:
    mIndex++;
}

StatementClassScope CppParser::getClassScope(const QString& text) {
    if (!text.isEmpty() && text[0]=='p') {
        if (text=="public")
            return StatementClassScope::Public;
        else if (text=="private")
            return StatementClassScope::Private;
        else if (text=="protected")
            return StatementClassScope::Protected;
    }
    return StatementClassScope::None;
}

StatementClassScope CppParser::getClassScope(KeywordType keywordType)
{
    switch(keywordType) {
    case KeywordType::Public:
        return StatementClassScope::Public;
    case KeywordType::Private:
        return StatementClassScope::Private;
    case KeywordType::Protected:
        return StatementClassScope::Protected;
    default:
        return StatementClassScope::None;
    }
}

void CppParser::handleScope(KeywordType keywordType)
{
    mClassScope = getClassScope(keywordType);
    mIndex+=2; // the scope is followed by a ':'
}


#ifdef QT_DEBUG
static int lastIndex=-1;
#endif

bool CppParser::handleStatement()
{
    QString funcType,funcName;
    int idx=getCurrentBlockEndSkip();
    int idx2=getCurrentBlockBeginSkip();
    int idx3=getCurrentInlineNamespaceEndSkip();
    KeywordType keywordType;
#ifdef QT_DEBUG
//    qDebug()<<lastIndex<<mIndex;
    Q_ASSERT(mIndex>=lastIndex);
    lastIndex=mIndex;
#endif

    if (mIndex >= idx2) {
        //skip (previous handled) block begin
        mBlockBeginSkips.pop_back();
        if (mIndex == idx2)
            mIndex++;
        else if (mIndex<mTokenizer.tokenCount())  //error happens, but we must remove an (error) added scope
            removeScopeLevel(mTokenizer[mIndex]->line);
    } else if (mIndex >= idx) {
        //skip (previous handled) block end
        mBlockEndSkips.pop_back();
        if (idx+1 < mTokenizer.tokenCount())
            removeScopeLevel(mTokenizer[idx+1]->line);
        if (mIndex == idx)
            mIndex++;
    } else if (mIndex >= idx3) {
        //skip (previous handled) inline name space end
        mInlineNamespaceEndSkips.pop_back();
        if (mIndex == idx3)
            mIndex++;
    } else if (mTokenizer[mIndex]->text.startsWith('{')) {
        PStatement block = addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    "",
                    "",
                    "",
                    "",
                    "",
                    //mTokenizer[mIndex]^.Line,
                    mTokenizer[mIndex]->line,
                    StatementKind::skBlock,
                    getScope(),
                    mClassScope,
                    StatementProperty::spHasDefinition);
        addSoloScopeLevel(block,mTokenizer[mIndex]->line,true);
        mIndex++;
    } else if (mTokenizer[mIndex]->text[0] == '}') {
        removeScopeLevel(mTokenizer[mIndex]->line);
        mIndex++;
    } else if (checkForPreprocessor()) {
        handlePreprocessor();
//    } else if (checkForLambda()) { // is lambda
//        handleLambda();
    } else if (mTokenizer[mIndex]->text=='(') {
        if (mIndex+1<mTokenizer.tokenCount() && mTokenizer[mIndex+1]->text=="operator") {
            // things like (operator int)
            mIndex++; //just skip '('
        } else
            skipParenthesis(mIndex);
    } else if (mTokenizer[mIndex]->text==')') {
        mIndex++;
    } else if (mTokenizer[mIndex]->text.startsWith('~')) {
        //it should be a destructor
        if (mIndex+1<mTokenizer.tokenCount()
                && mTokenizer[mIndex+1]->text=='(') {
            //dont further check to speed up
            handleMethod(StatementKind::skDestructor, "", '~'+mTokenizer[mIndex]->text, mIndex+1, false, false);
        } else {
            //error
            mIndex=moveToEndOfStatement(mIndex,false);
        }
    } else if (!isIdentChar(mTokenizer[mIndex]->text[0])) {
        mIndex=moveToEndOfStatement(mIndex,true);
    } else if (mTokenizer[mIndex]->text.endsWith('.')
               || mTokenizer[mIndex]->text.endsWith("->")) {
        mIndex=moveToEndOfStatement(mIndex,true);

    } else if (checkForKeyword(keywordType)) { // includes template now
        handleKeyword(keywordType);
    } else if (keywordType==KeywordType::For) { // (for/catch)
        handleForBlock();
    } else if (keywordType==KeywordType::Catch) { // (for/catch)
        handleCatchBlock();
    } else if (checkForScope(keywordType)) { // public /private/proteced
        handleScope(keywordType);
    } else if (keywordType==KeywordType::Enum) {
        handleEnum(false);
    } else if (keywordType==KeywordType::Typedef) {
        if (mIndex+1 < mTokenizer.tokenCount()) {
            if (checkForTypedefStruct()) { // typedef struct something
                mIndex++; // skip 'typedef'
                handleStructs(true);
            } else if (checkForTypedefEnum()) { // typedef enum something
                mIndex++; // skip 'typedef'
                handleEnum(true);
            } else
                handleOtherTypedefs(); // typedef Foo Bar
        } else
            mIndex++;
    } else if (checkForNamespace(keywordType)) {
        handleNamespace(keywordType);
    } else if (checkForUsing(keywordType)) {
        handleUsing();
    } else if (checkForStructs(keywordType)) {
        handleStructs(false);
    } else if (keywordType == KeywordType::Inline) {
        mIndex++;
    }else {
        // it should be method/constructor/var
        checkAndHandleMethodOrVar(keywordType);
    }
    //Q_ASSERT(mIndex<999999);

//    while (mTokenizer.lambdasCount()>0 && mTokenizer.indexOfFirstLambda()<mIndex) {
//        handleLambda(mTokenizer.indexOfFirstLambda());
//        mTokenizer.removeFirstLambda();
//    }
//    else if (checkForMethod(funcType, funcName, argStart,argEnd, isStatic, isFriend)) {
//        handleMethod(funcType, funcName, argStart, argEnd, isStatic, isFriend); // don't recalculate parts
//    } else if (tryHandleVar()) {
//        //do nothing
//    } else
//        mIndex++;

    //todo: remove mSkipList (we can check '}''s statement type instead)
//    checkForSkipStatement();

    return mIndex < mTokenizer.tokenCount();

}

void CppParser::handleStructs(bool isTypedef)
{
    bool isFriend = false;
    QString prefix = mTokenizer[mIndex]->text;
    if (prefix == "friend") {
        isFriend = true;
        mIndex++;
    }
    // Check if were dealing with a struct or union
    prefix = mTokenizer[mIndex]->text;
    bool isStruct = ("struct" == prefix) || ("union"==prefix);
    int startLine = mTokenizer[mIndex]->line;

    mIndex++; //skip struct/class/union

    if (mIndex>=mTokenizer.tokenCount())
        return;

    // Do not modifiy index
    int i=indexOfNextSemicolonOrLeftBrace(mIndex);
    if (i >= mTokenizer.tokenCount()) {
        //error
        mIndex=i;
        return;
    }
    // Forward class/struct decl *or* typedef, e.g. typedef struct some_struct synonym1, synonym2;
    if (mTokenizer[i]->text.front() == ';') {
        // typdef struct Foo Bar
        if (isTypedef) {
            QString oldType = mTokenizer[mIndex]->text;
            while(true) {
                // Add definition statement for the synonym
                if ((mIndex + 1 < mTokenizer.tokenCount())
                        && (mTokenizer[mIndex + 1]->text.front()==','
                            || mTokenizer[mIndex + 1]->text.front()==';')) {
                    QString newType = mTokenizer[mIndex]->text;
                    addStatement(
                                getCurrentScope(),
                                mCurrentFile,
                                oldType,
                                newType,
                                "", // args
                                "", // noname args
                                "", // values
                                startLine,
                                StatementKind::skTypedef,
                                getScope(),
                                mClassScope,
                                StatementProperty::spHasDefinition);
                }
                mIndex++;
                if (mIndex >= mTokenizer.tokenCount())
                    break;
                if (mTokenizer[mIndex]->text.front() == ';')
                    break;
            }
        } else {
            if (isFriend) { // friend class
                PStatement parentStatement = getCurrentScope();
                if (parentStatement) {
                    parentStatement->friends.insert(mTokenizer[mIndex]->text);
                }
            } else {
            // todo: Forward declaration, struct Foo. Don't mention in class browser
            }
            i++; // step over ;
            mIndex = i;
        }

        // normal class/struct decl
    } else {
        PStatement firstSynonym;
        // Add class/struct name BEFORE opening brace
        if (mTokenizer[mIndex]->text.front() != '{') {
            while(mIndex < mTokenizer.tokenCount()) {
                if (mTokenizer[mIndex]->text.front() == ':'
                        || mTokenizer[mIndex]->text.front() == '{'
                        || mTokenizer[mIndex]->text.front() == ';') {
                    break;
                } else if ((mIndex + 1 < mTokenizer.tokenCount())
                  && (mTokenizer[mIndex + 1]->text.front() == ','
                      || mTokenizer[mIndex + 1]->text.front() == ';'
                      || mTokenizer[mIndex + 1]->text.front() == '{'
                      || mTokenizer[mIndex + 1]->text.front() == ':')) {
                    QString command = mTokenizer[mIndex]->text;

                    PStatement scopeStatement=getCurrentScope();
                    QString scopelessName;
                    QString parentName;
                    if (splitLastMember(command,scopelessName,parentName)) {
                        scopeStatement = getIncompleteClass(parentName,getCurrentScope());
                    } else {
                        scopelessName=command;
                    }
                    if (!command.isEmpty()) {
                        firstSynonym = addStatement(
                                    scopeStatement,
                                    mCurrentFile,
                                    prefix, // type
                                    scopelessName, // command
                                    "", // args
                                    "", // no name args,
                                    "", // values
                                    startLine,
                                    StatementKind::skClass,
                                    getScope(),
                                    mClassScope,
                                    StatementProperty::spHasDefinition);
                        command = "";
                    }
                    mIndex++;
                    break;
                } else if ((mIndex + 2 < mTokenizer.tokenCount())
                           && (mTokenizer[mIndex + 1]->text == "final")
                           && (mTokenizer[mIndex + 2]->text.front()==','
                               || mTokenizer[mIndex + 2]->text.front()==':'
                               || isblockChar(mTokenizer[mIndex + 2]->text.front()))) {
                    QString command = mTokenizer[mIndex]->text;
                    if (!command.isEmpty()) {
                        firstSynonym = addStatement(
                                    getCurrentScope(),
                                    mCurrentFile,
                                    prefix, // type
                                    command, // command
                                    "", // args
                                    "", // no name args
                                    "", // values
                                    startLine,
                                    StatementKind::skClass,
                                    getScope(),
                                    mClassScope,
                                    StatementProperty::spHasDefinition);
                        command="";
                    }
                    mIndex+=2;
                    break;
                } else
                    mIndex++;
            }
        }

        // Walk to opening brace if we encountered inheritance statements
        if ((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.front() == ':')) {
            if (firstSynonym)
                setInheritance(mIndex, firstSynonym, isStruct); // set the _InheritanceList value
            mIndex=indexOfNextLeftBrace(mIndex);
        }

        // Check for struct synonyms after close brace
        if (isStruct) {

            // Walk to closing brace
            i = indexOfMatchingBrace(mIndex); // step onto closing brace

            if ((i + 1 < mTokenizer.tokenCount()) && !(
                        mTokenizer[i + 1]->text.front() == ';'
                        || mTokenizer[i + 1]->text.front() ==  '}')) {
                // When encountering names again after struct body scanning, skip it
                QString command = "";
                QString args = "";

                // Add synonym before opening brace
                while(true) {
                    i++;
                    if (mTokenizer[i]->text=='('
                            || mTokenizer[i]->text==')') {
                        //skip
                    } else if (!(mTokenizer[i]->text == '{'
                          || mTokenizer[i]->text == ','
                          || mTokenizer[i]->text == ';')) {
                        if (mTokenizer[i]->text.endsWith(']')) { // cut-off array brackets
                            int pos = mTokenizer[i]->text.indexOf('[');
                            command += mTokenizer[i]->text.mid(0,pos) + ' ';
                            args =  mTokenizer[i]->text.mid(pos);
                        } else if (mTokenizer[i]->text.front() == '*'
                                   || mTokenizer[i]->text.front() == '&') { // do not add spaces after pointer operator
                            command += mTokenizer[i]->text;
                        } else {
                            command += mTokenizer[i]->text + ' ';
                        }
                    } else {
                        command = command.trimmed();
                        if (!command.isEmpty() &&
                                ( !firstSynonym
                                  || command!=firstSynonym->command )) {
                            //not define the struct yet, we define a unamed struct
                            if (!firstSynonym) {
                                firstSynonym = addStatement(
                                            getCurrentScope(),
                                            mCurrentFile,
                                            prefix,
                                            "__"+command,
                                            "",
                                            "",
                                            "",
                                            startLine,
                                            StatementKind::skClass,
                                            getScope(),
                                            mClassScope,
                                            StatementProperty::spHasDefinition);
                            }
                            if (isTypedef) {
                                //typedef
                                addStatement(
                                            getCurrentScope(),
                                            mCurrentFile,
                                            firstSynonym->command,
                                            command,
                                            "",
                                            "",
                                            "",
                                            mTokenizer[mIndex]->line,
                                            StatementKind::skTypedef,
                                            getScope(),
                                            mClassScope,
                                            StatementProperty::spHasDefinition); // typedef
                            } else {
                                //variable define
                                addStatement(
                                  getCurrentScope(),
                                  mCurrentFile,
                                  firstSynonym->command,
                                  command,
                                  args,
                                  "",
                                  "",
                                  mTokenizer[i]->line,
                                  StatementKind::skVariable,
                                  getScope(),
                                  mClassScope,
                                  StatementProperty::spHasDefinition); // TODO: not supported to pass list
                            }
                        }
                        command = "";
                    }
                    if (i >= mTokenizer.tokenCount() - 1)
                        break;
                    if (mTokenizer[i]->text=='{'
                          || mTokenizer[i]->text== ';')
                        break;
                }

              // Nothing worth mentioning after closing brace
              // Proceed to set first synonym as current class
            }
        }
        if (!firstSynonym) {
            //anonymous union/struct/class, add as a block
            firstSynonym=addStatement(
                      getCurrentScope(),
                      mCurrentFile,
                      "",
                      "",
                      "",
                      "",
                      "",
                      startLine,
                      StatementKind::skBlock,
                      getScope(),
                      mClassScope,
                      StatementProperty::spHasDefinition);
        }
        addSoloScopeLevel(firstSynonym,startLine);

        // Step over {
        if ((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.front() == '{'))
            mIndex++;
    }
}

void CppParser::handleUsing()
{
    int startLine = mTokenizer[mIndex]->line;
    if (mCurrentFile.isEmpty()) {
        //skip pass next ;
        mIndex=indexOfNextSemicolon(mIndex)+1;
        return;
    }

    mIndex++; //skip 'using'

    //handle things like 'using vec = std::vector; '
    if (mIndex+1 < mTokenizer.tokenCount()
            && mTokenizer[mIndex+1]->text == "=") {
        QString fullName = mTokenizer[mIndex]->text;
        QString aliasName;
        mIndex+=2;
        while (mIndex<mTokenizer.tokenCount() &&
               mTokenizer[mIndex]->text!=';') {
            aliasName += mTokenizer[mIndex]->text;
            mIndex++;
        }
        addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    aliasName, // name of the alias (type)
                    fullName, // command
                    "", // args
                    "", // noname args
                    "", // values
                    startLine,
                    StatementKind::skTypedef,
                    getScope(),
                    mClassScope,
                    StatementProperty::spHasDefinition);
        // skip ;
        mIndex++;
        return;
    }
    //handle things like 'using std::vector;'
    if ((mIndex+2>=mTokenizer.tokenCount())
            || (mTokenizer[mIndex]->text != "namespace")) {
        int i= mTokenizer[mIndex]->text.lastIndexOf("::");
        if (i>=0) {
            QString fullName = mTokenizer[mIndex]->text;
            QString usingName = fullName.mid(i+2);
            addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        fullName, // name of the alias (type)
                        usingName, // command
                        "", // args
                        "", // noname args
                        "", // values
                        startLine,
                        StatementKind::skAlias,
                        getScope(),
                        mClassScope,
                        StatementProperty::spHasDefinition);
        }
        //skip to ; and skip it
        mIndex=indexOfNextSemicolon(mIndex)+1;
        return;
    }
    mIndex++;  // skip 'namespace'
    PStatement scopeStatement = getCurrentScope();

    QString usingName = mTokenizer[mIndex]->text;
    mIndex++;

    if (scopeStatement) {
        QString fullName = scopeStatement->fullName + "::" + usingName;
        if (!mNamespaces.contains(fullName)) {
            fullName = usingName;
        }
        if (mNamespaces.contains(fullName)) {
            scopeStatement->usingList.insert(fullName);
        }
    } else {
        PFileIncludes fileInfo = mPreprocessor.includesList().value(mCurrentFile);
        if (!fileInfo)
            return;
        if (mNamespaces.contains(usingName)) {
            fileInfo->usings.insert(usingName);
        }
    }
}

void CppParser::handleVar(const QString& typePrefix,bool isExtern,bool isStatic)
{
    QString lastType;
    if (typePrefix=="extern") {
        isExtern=true;
    } else if (typePrefix=="static") {
        isStatic=true;
    } else {
        lastType=typePrefix;
    }

    PStatement addedVar;

    QString tempType;
    while(mIndex<mTokenizer.tokenCount()) {
        // Skip bit identifiers,
        // e.g.:
        // handle
        // unsigned short bAppReturnCode:8,reserved:6,fBusy:1,fAck:1
        // as
        // unsigned short bAppReturnCode,reserved,fBusy,fAck
        if (mTokenizer[mIndex]->text.front() == ':') {
            //handle e.g.: for(auto x:vec)
            if (mIndex+1<mTokenizer.tokenCount()
                    && isIdentifier(mTokenizer[mIndex+1]->text)
                    && isIdentChar(mTokenizer[mIndex+1]->text.back())
                    && addedVar
                    && !(addedVar->properties & StatementProperty::spFunctionPointer)
                    && AutoTypes.contains(addedVar->type)) {
                QStringList phraseExpression;
                phraseExpression.append(mTokenizer[mIndex+1]->text);
                int pos = 0;
                PEvalStatement aliasStatement = doEvalExpression(mCurrentFile,
                                        phraseExpression,
                                        pos,
                                        getCurrentScope(),
                                        PEvalStatement(),
                                        true);
                if(aliasStatement && aliasStatement->effectiveTypeStatement
                        && STLContainers.contains(aliasStatement->effectiveTypeStatement->fullName)) {
                    if (STLMaps.contains(aliasStatement->effectiveTypeStatement->fullName)) {
                        addedVar->type = "std::pair"+aliasStatement->templateParams;
                    } else {
                        QString type=doFindFirstTemplateParamOf(mCurrentFile,aliasStatement->templateParams,
                                                              getCurrentScope());
                        if (!type.isEmpty())
                            addedVar->type = type;
                    }
                }
            }
            addedVar.reset();
            bool should_exit=false;
            while (mIndex < mTokenizer.tokenCount()) {
                switch(mTokenizer[mIndex]->text[0].unicode()) {
                case ',':
                case ';':
                case '=':
                    should_exit = true;
                    break;
                case ')':
                    mIndex++;
                    return;
                case '(':
                    mIndex=mTokenizer[mIndex]->matchIndex+1;
                    break;
                default:
                    mIndex++;
                }
                if (should_exit)
                    break;
            }
        } else if (mTokenizer[mIndex]->text==';') {
            break;
        } else if (isWordChar(mTokenizer[mIndex]->text[0])) {
            QString cmd=mTokenizer[mIndex]->text;
            if (mIndex+1< mTokenizer.tokenCount() && mTokenizer[mIndex+1]->text=='('
                    && mTokenizer[mIndex+1]->matchIndex+1<mTokenizer.tokenCount()
                    && mTokenizer[mTokenizer[mIndex+1]->matchIndex+1]->text=='(') {
                        //function pointer
                cmd = mTokenizer[mIndex+2]->text;
                int argStart=mTokenizer[mIndex+1]->matchIndex+1;
                int argEnd=mTokenizer[argStart]->matchIndex;
                if (cmd.startsWith("*"))
                    cmd = cmd.mid(1);

                if (!cmd.isEmpty()) {
                    QString type=lastType;
                    addedVar = addChildStatement(
                                getCurrentScope(),
                                mCurrentFile,
                                (lastType+" "+tempType+" "+mTokenizer[mIndex]->text).trimmed(),
                                cmd,
                                mergeArgs(argStart,argEnd),
                                "",
                                "",
                                mTokenizer[mIndex]->line,
                                StatementKind::skVariable,
                                getScope(),
                                mClassScope,
                                //True,
                                (isExtern?StatementProperty::spNone:StatementProperty::spHasDefinition)
                                | (isStatic?StatementProperty::spStatic:StatementProperty::spNone)
                                | StatementProperty::spFunctionPointer);
                } else {
                    addedVar.reset();
                }
                tempType="";
                mIndex=indexOfNextSemicolonOrLeftBrace(argEnd+1);
            }  else {
                //normal var
                while (cmd.startsWith('*')) {
                    cmd=cmd.mid(1);
                }
                if (cmd=="const") {
                    tempType=mTokenizer[mIndex]->text;
                } else {
                    QString suffix;
                    QString args;
                    cmd=mTokenizer[mIndex]->text;
                    parseCommandTypeAndArgs(cmd,suffix,args);
                    if (!cmd.isEmpty()) {
                        addedVar = addChildStatement(
                                    getCurrentScope(),
                                    mCurrentFile,
                                    (lastType+' '+tempType+suffix).trimmed(),
                                    cmd,
                                    args,
                                    "",
                                    "",
                                    mTokenizer[mIndex]->line,
                                    StatementKind::skVariable,
                                    getScope(),
                                    mClassScope,
                                    //True,
                                    (isExtern?StatementProperty::spNone:StatementProperty::spHasDefinition)
                                    | (isStatic?StatementProperty::spStatic:StatementProperty::spNone));
                        tempType="";
                    }
                }
                mIndex++;
            }
        } else if (mTokenizer[mIndex]->text=='=') {
            if (mIndex+1<mTokenizer.tokenCount()
                    && isIdentifier(mTokenizer[mIndex+1]->text)
                    && addedVar
                    && !(addedVar->properties & StatementProperty::spFunctionPointer)
                    && AutoTypes.contains(addedVar->type)) {
                int pos = 0;

                int endIndex = skipAssignment(mIndex, mTokenizer.tokenCount());
                QStringList phraseExpression;
                for (int i=mIndex+1;i<endIndex;i++) {
                    QString cmd = mTokenizer[i]->text;
                    if (cmd.length()>1 && cmd.endsWith(".")) {
                        phraseExpression.append(cmd.left(cmd.length()-1));
                        phraseExpression.append(".");
                    } else if (cmd.length()>2 && cmd.endsWith("->")) {
                        phraseExpression.append(cmd.left(cmd.length()-2));
                        phraseExpression.append("->");
                    } else if (cmd.length()>2 && cmd.endsWith("::")) {
                        phraseExpression.append(cmd.left(cmd.length()-2));
                        phraseExpression.append("::");
                    } else
                        phraseExpression.append(cmd);
                }
                PEvalStatement aliasStatement = doEvalExpression(mCurrentFile,
                                        phraseExpression,
                                        pos,
                                        getCurrentScope(),
                                        PEvalStatement(),
                                        true);
                if(aliasStatement) {
                    if (aliasStatement->effectiveTypeStatement) {
                        addedVar->type = aliasStatement->effectiveTypeStatement->fullName;
                        if (!addedVar->type.endsWith(">"))
                            addedVar->type += aliasStatement->templateParams;
                        if (aliasStatement->typeStatement
                                && STLIterators.contains(aliasStatement->typeStatement->command)
                                && !aliasStatement->templateParams.isEmpty()) {
                            PStatement parentStatement = aliasStatement->typeStatement->parentScope.lock();
                            if (parentStatement
                                    && STLContainers.contains(parentStatement->fullName)) {
                                addedVar->type = parentStatement->fullName+aliasStatement->templateParams+"::"+aliasStatement->typeStatement->command;
                            }
                        }
                    } else
                        addedVar->type = aliasStatement->baseType + aliasStatement->templateParams;
                    if (aliasStatement->pointerLevel>0)
                        addedVar->type += QString(aliasStatement->pointerLevel,'*');
                }
                mIndex = endIndex;
            } else
                mIndex = skipAssignment(mIndex, mTokenizer.tokenCount());
            addedVar.reset();
        } else if (mTokenizer[mIndex]->text=='{'
                   || mTokenizer[mIndex]->text=='(') {
            tempType="";
            if (mIndex+1<mTokenizer.tokenCount()
                    && isIdentifier(mTokenizer[mIndex+1]->text)
                    && addedVar
                    && !(addedVar->properties & StatementProperty::spFunctionPointer)
                    && AutoTypes.contains(addedVar->type)) {
                int pos = 0;
                int endIndex = mTokenizer[mIndex]->matchIndex;
                QStringList phraseExpression;
                for (int i=mIndex+1;i<endIndex;i++) {
                    QString cmd = mTokenizer[i]->text;
                    if (cmd.length()>1 && cmd.endsWith(".")) {
                        phraseExpression.append(cmd.left(cmd.length()-1));
                        phraseExpression.append(".");
                    } else if (cmd.length()>2 && cmd.endsWith("->")) {
                        phraseExpression.append(cmd.left(cmd.length()-2));
                        phraseExpression.append("->");
                    } else if (cmd.length()>2 && cmd.endsWith("::")) {
                        phraseExpression.append(cmd.left(cmd.length()-2));
                        phraseExpression.append("::");
                    } else
                        phraseExpression.append(cmd);
                }
                PEvalStatement aliasStatement = doEvalExpression(mCurrentFile,
                                        phraseExpression,
                                        pos,
                                        getCurrentScope(),
                                        PEvalStatement(),
                                        true);
                if(aliasStatement  && aliasStatement->effectiveTypeStatement) {
                    if (aliasStatement->effectiveTypeStatement) {
                        addedVar->type = aliasStatement->effectiveTypeStatement->fullName;
                        if (!addedVar->type.endsWith(">"))
                            addedVar->type += aliasStatement->templateParams;
                        if (aliasStatement->typeStatement
                                && STLIterators.contains(aliasStatement->typeStatement->command)) {
                            PStatement parentStatement = aliasStatement->typeStatement->parentScope.lock();
                            if (parentStatement
                                    && STLContainers.contains(parentStatement->fullName)) {
                                addedVar->type = parentStatement->fullName+aliasStatement->templateParams+"::"+aliasStatement->typeStatement->command;
                            }
                        }
                    } else
                        addedVar->type = aliasStatement->baseType + aliasStatement->templateParams;
                    if (aliasStatement->pointerLevel>0)
                        addedVar->type += QString(aliasStatement->pointerLevel,'*');
                }
            }
            mIndex=mTokenizer[mIndex]->matchIndex+1;
            addedVar.reset();
        } else {
            tempType="";
            mIndex++;
        }
    }
    // Skip ;
    mIndex++;
}

void CppParser::internalParse(const QString &fileName)
{
    // Perform some validation before we start
    if (!mEnabled)
        return;
//    if (!isCfile(fileName) && !isHfile(fileName))  // support only known C/C++ files
//        return;

    QStringList buffer;
    if (mOnGetFileStream) {
        mOnGetFileStream(fileName,buffer);
    }

    // Preprocess the file...
    {
        auto action = finally([this]{
            mTokenizer.clear();
        });
        // Let the preprocessor augment the include records
//        mPreprocessor.setIncludesList(mIncludesList);
//        mPreprocessor.setScannedFileList(mScannedFiles);
//        mPreprocessor.setIncludePaths(mIncludePaths);
//        mPreprocessor.setProjectIncludePaths(mProjectIncludePaths);
        mPreprocessor.setScanOptions(mParseGlobalHeaders, mParseLocalHeaders);
        mPreprocessor.preprocess(fileName, buffer);

        QStringList preprocessResult = mPreprocessor.result();
#ifdef QT_DEBUG
//        stringsToFile(mPreprocessor.result(),QString("r:\\preprocess-%1.txt").arg(extractFileName(fileName)));
//        mPreprocessor.dumpDefinesTo("r:\\defines.txt");
//        mPreprocessor.dumpIncludesListTo("r:\\includes.txt");
#endif
        //reduce memory usage
        mPreprocessor.clearTempResults();

        // Tokenize the preprocessed buffer file
        mTokenizer.tokenize(preprocessResult);
        //reduce memory usage
        preprocessResult.clear();
        if (mTokenizer.tokenCount() == 0)
            return;
#ifdef QT_DEBUG
//        mTokenizer.dumpTokens(QString("r:\\tokens-%1.txt").arg(extractFileName(fileName)));
#endif
#ifdef QT_DEBUG
        lastIndex = -1;
#endif
        // Process the token list
        while(true) {
            if (!handleStatement())
                break;
        }
#ifdef QT_DEBUG
//        mStatementList.dumpAll(QString("r:\\all-stats-%1.txt").arg(extractFileName(fileName)));
//        mStatementList.dump(QString("r:\\stats-%1.txt").arg(extractFileName(fileName)));
#endif
        //reduce memory usage
        internalClear();

    }
}

void CppParser::inheritClassStatement(const PStatement& derived, bool isStruct,
                                      const PStatement& base, StatementClassScope access)
{
    //differentiate class and struct
    if (access == StatementClassScope::None) {
        if (isStruct)
            access = StatementClassScope::Public;
        else
            access = StatementClassScope::Private;
    }
    foreach (const PStatement& statement, base->children) {
        if (statement->classScope == StatementClassScope::Private
                || statement->kind == StatementKind::skConstructor
                || statement->kind == StatementKind::skDestructor)
            continue;
        StatementClassScope m_acc;
        switch(access) {
        case StatementClassScope::Public:
            m_acc = statement->classScope;
            break;
        case StatementClassScope::Protected:
            m_acc = StatementClassScope::Protected;
            break;
        case StatementClassScope::Private:
            m_acc = StatementClassScope::Private;
            break;
        default:
            m_acc = StatementClassScope::Private;
        }
        //inherit
        addInheritedStatement(derived,statement,m_acc);
    }
}

void CppParser::fillListOfFunctions(const QString& fileName, int line,
                                    const PStatement& statement,
                                    const PStatement& scopeStatement, QStringList &list)
{
    StatementMap children = mStatementList.childrenStatements(scopeStatement);
    for (const PStatement& child:children) {
        if ((statement->command == child->command)
#ifdef Q_OS_WIN
                || (statement->command +'A' == child->command)
                || (statement->command +'W' == child->command)
#endif
                ) {
            if (line < child->line && (child->fileName == fileName))
                continue;
            list.append(prettyPrintStatement(child,child->fileName,child->line));
        }
    }
}

QList<PStatement> CppParser::getListOfFunctions(const QString &fileName, int line, const PStatement &statement, const PStatement &scopeStatement)
{
    QList<PStatement> result;
    StatementMap children = mStatementList.childrenStatements(scopeStatement);
    for (const PStatement& child:children) {
        if (( (statement->command == child->command)
#ifdef Q_OS_WIN
                || (statement->command +'A' == child->command)
                || (statement->command +'W' == child->command)
#endif
              ) ) {
            if (line < child->line && (child->fileName == fileName))
                continue;
            result.append(child);
        }
    }
    return result;
}

PStatement CppParser::findMemberOfStatement(const QString &phrase,
                                            const PStatement& scopeStatement)
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scopeStatement);
    if (statementMap.isEmpty())
        return PStatement();

    QString s = phrase;
    //remove []
    int p = phrase.indexOf('[');
    if (p>=0)
        s.truncate(p);
    //remove ()
    p = phrase.indexOf('(');
    if (p>=0)
        s.truncate(p);

    //remove <>
    p =s.indexOf('<');
    if (p>=0)
        s.truncate(p);

    return statementMap.value(s,PStatement());
}

QList<PStatement> CppParser::findMembersOfStatement(const QString &phrase, const PStatement &scopeStatement)
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scopeStatement);
    if (statementMap.isEmpty())
        return QList<PStatement>();

    QString s = phrase;
    //remove []
    int p = phrase.indexOf('[');
    if (p>=0)
        s.truncate(p);
    //remove ()
    p = phrase.indexOf('(');
    if (p>=0)
        s.truncate(p);

    //remove <>
    p =s.indexOf('<');
    if (p>=0)
        s.truncate(p);

    return statementMap.values(s);
}

PStatement CppParser::findStatementInScope(const QString &name, const QString &noNameArgs,
                                           StatementKind kind, const PStatement& scope)
{
    if (scope && scope->kind == StatementKind::skNamespace) {
        PStatementList namespaceStatementsList = findNamespace(scope->command);
        if (!namespaceStatementsList)
            return PStatement();
        foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
            PStatement result=doFindStatementInScope(name,noNameArgs,kind,namespaceStatement);
            if (result)
                return result;
        }
    } else {
        return doFindStatementInScope(name,noNameArgs,kind,scope);
    }
    return PStatement();
}

PStatement CppParser::findStatementInScope(const QString &name, const PStatement &scope)
{
    if (!scope)
        return findMemberOfStatement(name,scope);
    if (scope->kind == StatementKind::skNamespace) {
        return findStatementInNamespace(name, scope->fullName);
    } else {
        return findMemberOfStatement(name,scope);
    }
}

PStatement CppParser::findStatementInNamespace(const QString &name, const QString &namespaceName)
{
    PStatementList namespaceStatementsList=findNamespace(namespaceName);
    if (!namespaceStatementsList)
        return PStatement();
    foreach (const PStatement& namespaceStatement,*namespaceStatementsList) {
        PStatement result = findMemberOfStatement(name,namespaceStatement);
        if (result)
            return result;
    }
    return PStatement();
}

PEvalStatement CppParser::doEvalExpression(const QString& fileName,
                                       const QStringList& phraseExpression,
                                       int &pos,
                                       const PStatement& scope,
                                       const PEvalStatement& previousResult,
                                       bool freeScoped)
{
    //dummy function to easy later upgrades
    return doEvalPointerArithmetic(fileName,
                                        phraseExpression,
                                        pos,
                                        scope,
                                        previousResult,
                                   freeScoped);
}

PEvalStatement CppParser::doEvalPointerArithmetic(const QString &fileName, const QStringList &phraseExpression, int &pos, const PStatement &scope, const PEvalStatement &previousResult, bool freeScoped)
{
    if (pos>=phraseExpression.length())
        return PEvalStatement();
    //find the start scope statement
    PEvalStatement currentResult = doEvalPointerToMembers(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    while (pos < phraseExpression.length()) {
        if (!currentResult)
            break;
        if (currentResult &&
                (phraseExpression[pos]=="+"
                    || phraseExpression[pos]=="-")) {
            if (currentResult->kind == EvalStatementKind::Variable) {
                pos++;
                PEvalStatement op2=doEvalPointerToMembers(
                            fileName,
                            phraseExpression,
                            pos,
                            scope,
                            currentResult,
                            false);
                //todo operator+/- overload
            } else if (currentResult->kind == EvalStatementKind::Literal
                       && currentResult->baseType == "int") {
                pos++;
                PEvalStatement op2=doEvalPointerToMembers(
                            fileName,
                            phraseExpression,
                            pos,
                            scope,
                            currentResult,
                            false);
                currentResult = op2;
            } else
                break;
        } else
            break;
    }
//    qDebug()<<pos<<"pointer add member end";
    return currentResult;

}

PEvalStatement CppParser::doEvalPointerToMembers(
        const QString &fileName,
        const QStringList &phraseExpression,
        int &pos,
        const PStatement &scope,
        const PEvalStatement &previousResult,
        bool freeScoped)
{
    if (pos>=phraseExpression.length())
        return PEvalStatement();
    //find the start scope statement
    PEvalStatement currentResult = doEvalCCast(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    while (pos < phraseExpression.length()) {
        if (!currentResult)
            break;
        if (currentResult &&
                (currentResult->kind == EvalStatementKind::Variable)
                && (phraseExpression[pos]==".*"
                    || phraseExpression[pos]=="->*")) {
            pos++;
            currentResult =
                    doEvalCCast(
                        fileName,
                        phraseExpression,
                        pos,
                        scope,
                        currentResult,
                        false);
            if (currentResult) {
                currentResult->pointerLevel++;
            }
        } else
            break;
    }
//    qDebug()<<pos<<"pointer member end";
    return currentResult;
}

PEvalStatement CppParser::doEvalCCast(const QString &fileName,
                                                   const QStringList &phraseExpression,
                                                   int &pos,
                                                   const PStatement& scope,
                                                   const PEvalStatement& previousResult,
                                                   bool freeScoped)
{
    if (pos>=phraseExpression.length())
        return PEvalStatement();
    PEvalStatement result;
    if (phraseExpression[pos]=="*") {
        pos++; //skip "*"
        result = doEvalCCast(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    previousResult,
                    freeScoped);
        if (result) {
            //todo: STL container;

            if (result->pointerLevel==0) {
                //STL smart pointers
                if (result->typeStatement
                        && STLIterators.contains(result->typeStatement->command)
                ) {
                    PStatement parentScope = result->typeStatement->parentScope.lock();
                    if (STLContainers.contains(parentScope->fullName)) {
                        QString typeName=findFirstTemplateParamOf(fileName,result->templateParams, parentScope);
    //                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                        PStatement typeStatement=findTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateEvalType(fileName,typeStatement);
                            result->kind = EvalStatementKind::Variable;
                        }
                    }
                } else {
                    PStatement typeStatement = result->effectiveTypeStatement;
                    if ((typeStatement)
                        && STLPointers.contains(typeStatement->fullName)
                        && result->kind == EvalStatementKind::Variable
                        && result->baseStatement) {
                        PStatement parentScope = result->baseStatement->parentScope.lock();
                        QString typeName=findFirstTemplateParamOf(fileName,result->baseStatement->type, parentScope);
    //                    qDebug()<<"typeName"<<typeName;
                        typeStatement=findTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateEvalType(fileName,typeStatement);
                            result->kind = EvalStatementKind::Variable;
                        }
                    }
                }
            } else
                result->pointerLevel--;
        }
    } else if (phraseExpression[pos]=="&") {
        pos++; //skip "&"
        result = doEvalCCast(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    previousResult,
                    freeScoped);
        if (result) {
            result->pointerLevel++;
        }
    } else if (phraseExpression[pos]=="++"
               || phraseExpression[pos]=="--") {
        pos++; //skip "++" or "--"
        result = doEvalCCast(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    previousResult,
                    freeScoped);
    } else if (phraseExpression[pos]=="(") {
        //parse
        int startPos = pos;
        pos++;
//        qDebug()<<"parse type cast ()";
        PEvalStatement evalType = doEvalExpression(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    PEvalStatement(),
                    true);
//        qDebug()<<pos;
        if (pos >= phraseExpression.length() || phraseExpression[pos]!=")") {
            return PEvalStatement();
        } else if (evalType &&
                   (evalType->kind == EvalStatementKind::Type)) {
            pos++; // skip ")"
//            qDebug()<<"parse type cast exp";
            //it's a type cast
            result = doEvalCCast(fileName,
                        phraseExpression,
                        pos,
                        scope,
                        previousResult,
                        freeScoped);
            if (result) {
//                qDebug()<<"type cast";
                result->assignType(evalType);
            }
        }   else //it's not a type cast
            result = doEvalMemberAccess(
                        fileName,
                        phraseExpression,
                        startPos, //we must reparse it
                        scope,
                        previousResult,
                        freeScoped);
    } else
        result = doEvalMemberAccess(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
//    if (result) {
//        qDebug()<<pos<<(int)result->kind<<result->baseType;
//    } else {
//        qDebug()<<"!!!!!!!!!!!not found";
//    }
    return result;
}

PEvalStatement CppParser::doEvalMemberAccess(const QString &fileName,
                                                   const QStringList &phraseExpression,
                                                   int &pos,
                                                   const PStatement& scope,
                                                   const PEvalStatement& previousResult,
                                                   bool freeScoped)

{
//    qDebug()<<"eval member access "<<pos<<phraseExpression;
    PEvalStatement result;
    if (pos>=phraseExpression.length())
        return result;
    PEvalStatement lastResult = previousResult;
    result = doEvalScopeResolution(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    if (!result)
        return PEvalStatement();
    while (pos<phraseExpression.length()) {
        if (!result)
            break;
        if (phraseExpression[pos]=="++" || phraseExpression[pos]=="--") {
            pos++; //just skip it
        } else if (phraseExpression[pos] == "(") {
            if (result->kind == EvalStatementKind::Type) {
                pos++; // skip "("
                PEvalStatement newResult = doEvalExpression(
                            fileName,
                            phraseExpression,
                            pos,
                            scope,
                            PEvalStatement(),
                            true);
                if (newResult)
                    newResult->assignType(result);
                pos++; // skip ")"
                result = newResult;
            } else if (result->kind == EvalStatementKind::Function) {
                doSkipInExpression(phraseExpression,pos,"(",")");
//                qDebug()<<"????"<<(result->baseStatement!=nullptr)<<(lastResult!=nullptr);
                if (result->baseStatement && lastResult && lastResult->baseStatement) {
                    PStatement parentScope = result->baseStatement->parentScope.lock();
                    if (parentScope
                        && STLContainers.contains(parentScope->fullName)
                        && STLElementMethods.contains(result->baseStatement->command)
                    ) {
                    //stl container methods
                        PStatement typeStatement = result->effectiveTypeStatement;
                        QString typeName=findFirstTemplateParamOf(fileName,lastResult->baseStatement->type, parentScope);
//                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                        typeStatement=findTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateEvalType(fileName,typeStatement);
                            result->kind = EvalStatementKind::Variable;
                        }
                    }
                }
                result->kind = EvalStatementKind::Variable;
            } else
                result = PEvalStatement();
        } else if (phraseExpression[pos] == "[") {
            //skip to "]"
            doSkipInExpression(phraseExpression,pos,"[","]");
            if (result->pointerLevel>0)
                result->pointerLevel--;
            else {
                PStatement typeStatement = result->effectiveTypeStatement;
                if (typeStatement
                        && STLContainers.contains(typeStatement->fullName)
                        && result->kind == EvalStatementKind::Variable
                        && result->baseStatement) {
                    PStatement parentScope = result->baseStatement->parentScope.lock();
                    QString typeName = findFirstTemplateParamOf(fileName,result->baseStatement->type,
                                                    parentScope);
                    typeStatement = findTypeDefinitionOf(fileName, typeName,
                                                     parentScope);
                    if (typeStatement) {
                        result = doCreateEvalType(fileName,typeStatement);
                        result->kind = EvalStatementKind::Variable;
                    }
                }
            }
        } else if (phraseExpression[pos] == ".") {
            pos++;
            lastResult = result;
            result = doEvalScopeResolution(
                        fileName,
                        phraseExpression,
                        pos,
                        scope,
                        result,
                        false);
//            qDebug()<<(result!=nullptr)<<pos<<"after .";
        } else if (phraseExpression[pos] == "->") {
            pos++;
//            qDebug()<<"pointer level"<<result->pointerLevel;
            if (result->pointerLevel==0) {
                PStatement typeStatement = result->effectiveTypeStatement;
                if ((typeStatement)
                        && STLPointers.contains(typeStatement->fullName)
                        && result->kind == EvalStatementKind::Variable
                        && result->baseStatement) {
                    PStatement parentScope = result->baseStatement->parentScope.lock();
                    QString typeName=findFirstTemplateParamOf(fileName,result->baseStatement->type, parentScope);
//                    qDebug()<<"typeName"<<typeName;
                    typeStatement=findTypeDefinitionOf(fileName, typeName,parentScope);
                    if (typeStatement) {
                        result = doCreateEvalType(fileName,typeStatement);
                        result->kind = EvalStatementKind::Variable;
                    }
                }
            } else {
                result->pointerLevel--;
            }
            lastResult = result;
            result = doEvalScopeResolution(
                        fileName,
                        phraseExpression,
                        pos,
                        scope,
                        result,
                        false);
        } else
            break;
    }
    return result;
}

PEvalStatement CppParser::doEvalScopeResolution(const QString &fileName,
                                                   const QStringList &phraseExpression,
                                                   int &pos,
                                                   const PStatement& scope,
                                                   const PEvalStatement& previousResult,
                                                   bool freeScoped)
{
//    qDebug()<<"eval scope res "<<pos<<phraseExpression;
    PEvalStatement result;
    if (pos>=phraseExpression.length())
        return result;
    result = doEvalTerm(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    while (pos<phraseExpression.length()) {
        if (phraseExpression[pos]=="::" ) {
            pos++;
            if (!result) {
                //global
                result = doEvalTerm(fileName,
                                    phraseExpression,
                                    pos,
                                    PStatement(),
                                    PEvalStatement(),
                                    false);
            } else if (result->kind == EvalStatementKind::Type) {
                //class static member
                result = doEvalTerm(fileName,
                                    phraseExpression,
                                    pos,
                                    scope,
                                    result,
                                    false);
            } else if (result->kind == EvalStatementKind::Namespace) {
                //namespace
                result = doEvalTerm(fileName,
                                    phraseExpression,
                                    pos,
                                    scope,
                                    result,
                                    false);
            }
            if (!result)
                break;
        } else
            break;
    }
//    qDebug()<<pos<<"scope end";
    return result;
}

PEvalStatement CppParser::doEvalTerm(const QString &fileName,
                                                   const QStringList &phraseExpression,
                                                   int &pos,
                                                   const PStatement& scope,
                                                   const PEvalStatement& previousResult,
                                                   bool freeScoped)
{
//    if (previousResult) {
//        qDebug()<<"eval term "<<pos<<phraseExpression<<previousResult->baseType<<freeScoped;
//    } else {
//        qDebug()<<"eval term "<<pos<<phraseExpression<<"no type"<<freeScoped;
//    }
    PEvalStatement result;
    if (pos>=phraseExpression.length())
        return result;
    if (phraseExpression[pos]=="(") {
        pos++;
        result = doEvalExpression(fileName,phraseExpression,pos,scope,PEvalStatement(),freeScoped);
        if (pos >= phraseExpression.length() || phraseExpression[pos]!=")")
            return PEvalStatement();
        else {
            pos++; // skip ")";
            return result;
        }
    } else {
        int pointerLevel = 0;
        //skip "struct", "const", "static", etc
        while(pos < phraseExpression.length()) {
            QString token = phraseExpression[pos];
            if (token=="*") // for expression like (const * char)?
                pointerLevel++;
            else if (mCppTypeKeywords.contains(token)
                    || !mCppKeywords.contains(token))
                break;
            pos++;
        }

        if (pos>=phraseExpression.length() || phraseExpression[pos]==")")
            return result;
        if (mCppKeywords.contains(phraseExpression[pos])) {
            result = doCreateEvalType(phraseExpression[pos]);
            pos++;
        } else if (isIdentifier(phraseExpression[pos])) {
            PStatement statement;
            if (freeScoped) {
                if (!previousResult) {
                    statement = findStatementStartingFrom(
                                fileName,
                                phraseExpression[pos],
                                scope);
                } else {
                    statement = findStatementStartingFrom(
                                fileName,
                                phraseExpression[pos],
                                previousResult->effectiveTypeStatement);
                }
            } else {
                if (!previousResult) {
                    statement = findStatementInScope(phraseExpression[pos],PStatement());
                } else {
                    statement = findStatementInScope(phraseExpression[pos],previousResult->effectiveTypeStatement);
                }
            }
            pos++;
            if (statement && statement->kind == StatementKind::skConstructor) {
                statement = statement->parentScope.lock();
            }
            if (statement) {
                switch (statement->kind) {
                case StatementKind::skNamespace:
                    result = doCreateEvalNamespace(statement);
                    break;
                case StatementKind::skAlias: {
                    statement = findAliasedStatement(statement);
                    if (statement)
                        result = doCreateEvalNamespace(statement);
                }
                    break;
                case StatementKind::skVariable:
                case StatementKind::skParameter:
                    result = doCreateEvalVariable(fileName,statement, previousResult?previousResult->templateParams:"",scope);
                    break;
                case StatementKind::skEnumType:
                case StatementKind::skClass:
                case StatementKind::skEnumClassType:
                case StatementKind::skTypedef:
                    result = doCreateEvalType(fileName,statement);
                    break;
                case StatementKind::skFunction:
                    result = doCreateEvalFunction(fileName,statement);
                    break;
                default:
                    result = PEvalStatement();
                }
                if (result
                        && result->typeStatement
                        && STLIterators.contains(result->typeStatement->command)
                        && previousResult) {
                    PStatement parentStatement = result->typeStatement->parentScope.lock();
                    if (parentStatement
                            && STLContainers.contains(parentStatement->fullName)) {
                        result->templateParams = previousResult->templateParams;
                    }
                }
            }
        } else if (isIntegerLiteral(phraseExpression[pos])) {
            result = doCreateEvalLiteral("int");
            pos++;
        } else if (isFloatLiteral(phraseExpression[pos])) {
            result = doCreateEvalLiteral("double");
            pos++;
        } else if (isStringLiteral(phraseExpression[pos])) {
            result = doCreateEvalLiteral("char");
            result->pointerLevel = 1;
            pos++;
        } else if (isCharLiteral(phraseExpression[pos])) {
            result = doCreateEvalLiteral("char");
            pos++;
        } else
            return result;
//        if (result) {
//            qDebug()<<"term kind:"<<(int)result->kind;
//        }
        if (result && result->kind == EvalStatementKind::Type) {
            //skip "struct", "const", "static", etc
            while(pos < phraseExpression.length()) {
                QString token = phraseExpression[pos];
                if (token=="*") // for expression like (const * char)?
                    pointerLevel++;
                else if (mCppTypeKeywords.contains(token)
                        || !mCppKeywords.contains(token))
                    break;
                pos++;
            }
            result->pointerLevel = pointerLevel;
        }
    }
//    qDebug()<<pos<<" term end";
//    if (!result) {
//        qDebug()<<"not found !!!!";
//    }

    return result;
}

PEvalStatement CppParser::doCreateEvalNamespace(const PStatement &namespaceStatement)
{
    if (!namespaceStatement)
        return PEvalStatement();
    return std::make_shared<EvalStatement>(
                namespaceStatement->fullName,
                EvalStatementKind::Namespace,
                PStatement(),
                namespaceStatement,
                namespaceStatement);
}

PEvalStatement CppParser::doCreateEvalType(const QString& fileName,const PStatement &typeStatement)
{
    if (!typeStatement)
        return PEvalStatement();
    if (typeStatement->kind == StatementKind::skTypedef) {
        QString baseType;
        int pointerLevel=0;
        QString templateParams;
        PStatement tempStatement;
        PStatement effetiveTypeStatement  = doParseEvalTypeInfo(
                    fileName,
                    typeStatement->parentScope.lock(),
                    typeStatement->type + typeStatement->args,
                    baseType,
                    tempStatement,
                    pointerLevel,
                    templateParams);
        return std::make_shared<EvalStatement>(
                    baseType,
                    EvalStatementKind::Type,
                    PStatement(),
                    typeStatement,
                    effetiveTypeStatement,
                    pointerLevel,
                    templateParams
                    );
    } else {
        return std::make_shared<EvalStatement>(
                typeStatement->fullName,
                EvalStatementKind::Type,
                PStatement(),
                typeStatement,
                typeStatement);
    }
}

PEvalStatement CppParser::doCreateEvalType(const QString &primitiveType)
{
    return std::make_shared<EvalStatement>(
                primitiveType,
                EvalStatementKind::Type,
                PStatement(),
                PStatement(),
                PStatement());
}

PEvalStatement CppParser::doCreateEvalVariable(
        const QString &fileName,
        const PStatement& varStatement,
        const QString& baseTemplateParams,
        const PStatement& scope)
{
    if (!varStatement)
        return PEvalStatement();
    QString baseType;
    int pointerLevel=0;
    QString templateParams;
    PStatement typeStatement;
    PStatement effectiveTypeStatement;
    //todo: ugly implementation for std::pair
    if (varStatement->fullName == "std::pair::first") {
        effectiveTypeStatement = doParseEvalTypeInfo(
                fileName,
                scope,
                doFindFirstTemplateParamOf(fileName,baseTemplateParams,scope),
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);
    } else if (varStatement->fullName == "std::pair::second") {
        effectiveTypeStatement = doParseEvalTypeInfo(
                fileName,
                scope,
                doFindTemplateParamOf(fileName,baseTemplateParams,1,scope),
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);

    } else {
        effectiveTypeStatement = doParseEvalTypeInfo(
                fileName,
                varStatement->parentScope.lock(),
                varStatement->type+varStatement->args,
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);
    }
//    qDebug()<<"parse ..."<<baseType<<pointerLevel;
    return std::make_shared<EvalStatement>(
                baseType,
                EvalStatementKind::Variable,
                varStatement,
                typeStatement,
                effectiveTypeStatement,
                pointerLevel,
                templateParams
                );
}

PEvalStatement CppParser::doCreateEvalFunction(
        const QString &fileName,
        const PStatement& funcStatement)
{
    if (!funcStatement)
        return PEvalStatement();
    QString baseType;
    int pointerLevel=0;
    QString templateParams;
    PStatement typeStatement;
    PStatement effetiveTypeStatement = doParseEvalTypeInfo(
                fileName,
                funcStatement->parentScope.lock(),
                funcStatement->type,
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);
    return std::make_shared<EvalStatement>(
                baseType,
                EvalStatementKind::Function,
                funcStatement,
                typeStatement,
                effetiveTypeStatement,
                pointerLevel,
                templateParams
                );
}

PEvalStatement CppParser::doCreateEvalLiteral(const QString &type)
{
    return std::make_shared<EvalStatement>(
                type,
                EvalStatementKind::Literal,
                PStatement(),
                PStatement(),
                PStatement());
}

void CppParser::doSkipInExpression(const QStringList &expression, int &pos, const QString &startSymbol, const QString &endSymbol)
{
    int level = 0;
    while (pos<expression.length()) {
        QString token = expression[pos];
        if (token == startSymbol) {
            level++;
        } else if (token == endSymbol) {
            level--;
            if (level==0) {
                pos++;
                return;
            }
        }
        pos++;
    }
}

PStatement CppParser::doParseEvalTypeInfo(
        const QString &fileName,
        const PStatement &scope,
        const QString &type,
        QString &baseType,
        PStatement& typeStatement,
        int &pointerLevel,
        QString& templateParams)
{
    // Remove pointer stuff from type
    QString s = type;
//    qDebug()<<"eval type info"<<type;
    int position = s.length()-1;
    QSynedit::CppHighlighter highlighter;
    highlighter.resetState();
    highlighter.setLine(type,0);
    int bracketLevel = 0;
    int templateLevel  = 0;
    while(!highlighter.eol()) {
        QString token = highlighter.getToken();
        if (bracketLevel == 0 && templateLevel ==0) {
            if (token == "*")
                pointerLevel++;
            else if (token == "&")
                pointerLevel--;
            else if (highlighter.getTokenAttribute()->tokenType() == QSynedit::TokenType::Identifier) {
                baseType += token;
            } else if (token == "[") {
                pointerLevel++;
                bracketLevel++;
            } else if (token == "<") {
                templateLevel++;
                templateParams += token;
            } else if (token == "::") {
                baseType += token;
            }
        } else if (bracketLevel > 0) {
            if (token == "[") {
                bracketLevel++;
            } else if (token == "]") {
                bracketLevel--;
            }
        } else if (templateLevel > 0) {
            if (token == "<") {
                templateLevel++;
            } else if (token == ">") {
                templateLevel--;
            }
            templateParams += token;
        }
        highlighter.next();
    }
    while ((position >= 0) && (s[position] == '*'
                               || s[position] == ' '
                               || s[position] == '&')) {
        if (s[position]=='*') {

        }
        position--;
    }
    typeStatement = findStatementOf(fileName,baseType,scope,true);
    return getTypeDef(typeStatement,fileName,baseType);
}

int CppParser::getBracketEnd(const QString &s, int startAt)
{
    int i = startAt;
    int level = 0; // assume we start on top of [
    while (i < s.length()) {
        switch(s[i].unicode()) {
        case '<':
            level++;
            break;
        case '>':
            level--;
            if (level == 0)
                return i;
        }
        i++;
    }
    return startAt;
}

PStatement CppParser::doFindStatementInScope(const QString &name,
                                             const QString &noNameArgs,
                                             StatementKind kind,
                                             const PStatement& scope)
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scope);

    foreach (const PStatement& statement, statementMap.values(name)) {
        if (statement->kind == kind && statement->noNameArgs == noNameArgs) {
            return statement;
        }
    }
    return PStatement();
}

void CppParser::internalInvalidateFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return;

    // remove its include files list
    PFileIncludes p = findFileIncludes(fileName, true);
    if (p) {
        //fPreprocessor.InvalidDefinesInFile(FileName); //we don't need this, since we reset defines after each parse
        //p->includeFiles.clear();
        //p->usings.clear();
        for (PStatement& statement:p->statements) {
            if (statement->fileName==fileName) {
                mStatementList.deleteStatement(statement);
            } else {
                statement->setHasDefinition(false);
                statement->definitionFileName = statement->fileName;
                statement->definitionLine = statement->line;
            }
        }
        p->statements.clear();
    }

    //remove all statements from namespace cache
    const QList<QString>& keys=mNamespaces.keys();
    for (const QString& key:keys) {
        PStatementList statements = mNamespaces.value(key);
        for (int i=statements->size()-1;i>=0;i--) {
            PStatement statement = statements->at(i);
            if (statement->fileName == fileName) {
                statements->removeAt(i);
            }
        }
        if (statements->isEmpty()) {
            mNamespaces.remove(key);
        }
    }

    // delete it from scannedfiles
    mPreprocessor.removeScannedFile(fileName);
}

void CppParser::internalInvalidateFiles(const QSet<QString> &files)
{
    for (const QString& file:files)
        internalInvalidateFile(file);
}

QSet<QString> CppParser::calculateFilesToBeReparsed(const QString &fileName)
{
    if (fileName.isEmpty())
        return QSet<QString>();
    QSet<QString> result;
    result.insert(fileName);
    foreach (const QString& file, mProjectFiles) {
        PFileIncludes fileIncludes = mPreprocessor.includesList().value(file,PFileIncludes());
        if (fileIncludes && fileIncludes->includeFiles.contains(fileName)) {
            result.insert(file);
        }
    }
    return result;
}

int CppParser::calcKeyLenForStruct(const QString &word)
{
    if (word.startsWith("struct"))
        return 6;
    else if (word.startsWith("class")
             || word.startsWith("union"))
        return 5;
    return -1;
}

void CppParser::scanMethodArgs(const PStatement& functionStatement, int argStart)
{
    Q_ASSERT(mTokenizer[argStart]->text=='(');
    int argEnd=mTokenizer[argStart]->matchIndex;
    int paramStart = argStart+1;
    int i = paramStart ; // assume it starts with ( and ends with )
    // Keep going and stop on top of the variable name
    QString varType = "";
    while (i < argEnd) {
        if (mTokenizer[i]->text=='('
                && mTokenizer[i]->matchIndex+1<argEnd
                && mTokenizer[mTokenizer[i]->matchIndex+1]->text=='(') {
            //function pointer
            int argStart=mTokenizer[i]->matchIndex+1;
            int argEnd=mTokenizer[argStart]->matchIndex;
            QString cmd=mTokenizer[i+1]->text;
            if (cmd.startsWith('*'))
                cmd=cmd.mid(1);
            QString args=mergeArgs(argStart,argEnd);
            if (!cmd.isEmpty()) {
                addStatement(
                            functionStatement,
                            mCurrentFile,
                            varType, // 'int*'
                            cmd, // a
                            args,
                            "",
                            "",
                            mTokenizer[i+1]->line,
                            StatementKind::skParameter,
                            StatementScope::Local,
                            StatementClassScope::None,
                            StatementProperty::spHasDefinition);
            }
            i=argEnd+1;
            varType="";
        } else if (mTokenizer[i]->text=='{') {
            i=mTokenizer[i]->matchIndex+1;
        } else if (mTokenizer[i]->text=='(') {
            i=mTokenizer[i]->matchIndex+1;
        } else if (mTokenizer[i]->text=='=') {
            i=skipAssignment(i,argEnd);
        } else if (isWordChar(mTokenizer[i]->text[0])) {
            QString cmd=mTokenizer[i]->text;
            if (i+1==argEnd || mTokenizer[i+1]->text==',' || mTokenizer[i+1]->text=='=') {
                bool noCmd=false;
                if (!cmd.startsWith('*')
                        && !cmd.startsWith('&')
                        && !cmd.endsWith(']')) {
                    PStatement statement=findStatementOf(mCurrentFile,cmd,functionStatement,true);
                    noCmd = (statement && isTypeStatement(statement->kind));
                    if (!noCmd) {
                        QString args,suffix;
                        parseCommandTypeAndArgs(cmd,suffix,args);
                        if (!cmd.isEmpty()) {
                            addStatement(
                                        functionStatement,
                                        mCurrentFile,
                                        varType+suffix, // 'int*'
                                        cmd, // a
                                        args,
                                        "",
                                        "",
                                        mTokenizer[i]->line,
                                        StatementKind::skParameter,
                                        StatementScope::Local,
                                        StatementClassScope::None,
                                        StatementProperty::spHasDefinition);
                        }
                    }
                }
            } else {
                if (!varType.isEmpty())
                    varType+=' ';
                varType+=cmd;
            }
            i++;
        } else {
            i++;
            varType="";
        }
    }


}

QString CppParser::splitPhrase(const QString &phrase, QString &sClazz,
                               QString &sOperator, QString &sMember)
{
    sClazz="";
    sMember="";
    sOperator="";
    QString result="";
    int bracketLevel = 0;
    // Obtain stuff before first operator
    int firstOpStart = phrase.length() + 1;
    int firstOpEnd = phrase.length() + 1;
    for (int i = 0; i<phrase.length();i++) {
        if ((i+1<phrase.length()) && (phrase[i] == '-') && (phrase[i + 1] == '>') && (bracketLevel==0)) {
            firstOpStart = i;
            firstOpEnd = i+2;
            sOperator = "->";
            break;
        } else if ((i+1<phrase.length()) && (phrase[i] == ':') && (phrase[i + 1] == ':') && (bracketLevel==0)) {
            firstOpStart = i;
            firstOpEnd = i+2;
            sOperator = "::";
            break;
        } else if ((phrase[i] == '.') && (bracketLevel==0)) {
            firstOpStart = i;
            firstOpEnd = i+1;
            sOperator = ".";
            break;
        } else if (phrase[i] == '[') {
            bracketLevel++;
        } else if (phrase[i] == ']') {
            bracketLevel--;
        }
    }
    sClazz = phrase.mid(0, firstOpStart);
    if (firstOpStart == 0) {
        sMember = "";
        return "";
    }

    result = phrase.mid(firstOpEnd);

    // ... and before second op, if there is one
    int secondOp = 0;
    bracketLevel = 0;
    for (int i = firstOpEnd; i<phrase.length();i++) {
        if ((i+1<phrase.length()) && (phrase[i] == '-') && (phrase[i + 1] == '>') && (bracketLevel=0)) {
            secondOp = i;
            break;
        } else if ((i+1<phrase.length()) && (phrase[i] == ':') && (phrase[i + 1] == ':') && (bracketLevel=0)) {
            secondOp = i;
            break;
        } else if ((phrase[i] == '.') && (bracketLevel=0)) {
            secondOp = i;
            break;
        } else if (phrase[i] == '[') {
            bracketLevel++;
        } else if (phrase[i] == ']') {
            bracketLevel--;
        }
    }
    if (secondOp == 0) {
        sMember = phrase.mid(firstOpEnd);
    } else {
        sMember = phrase.mid(firstOpEnd,secondOp-firstOpEnd);
    }
    return result;
}

QString CppParser::removeTemplateParams(const QString &phrase)
{
    int pos = phrase.indexOf('<');
    if (pos>=0) {
        return phrase.left(pos);
    }
    return phrase;
}

bool CppParser::splitLastMember(const QString &token, QString &lastMember, QString &remaining)
{
    int pos = token.length()-1;
    int level=0;
    bool found=false;
    while (pos>=0 && !found) {
        switch(token[pos].unicode()) {
        case ']':
        case '>':
        case ')':
            level++;
            break;
        case '[':
        case '<':
        case '(':
            level--;
            break;
        case ':':
            if (level==0 && pos>0 && token[pos-1]==':') {
                found=true;
                break;
            }
        }
        pos--;
    }
    if (pos<0)
        return false;
    lastMember=token.mid(pos+2);
    remaining=token.left(pos);
    return true;
}

//static void appendArgWord(QString& args, const QString& word) {
//    QString s=word.trimmed();
//    if (s.isEmpty())
//        return;
//    if (args.isEmpty())
//        args.append(s);
//    else if (isIdentChar(args.back()) && isIdentChar(word.front()) ) {
//        args+=" ";
//        args+=s;
//    } else {
//        args+=s;
//    }
//}

bool CppParser::isNotFuncArgs(int startIndex)
{
    Q_ASSERT(mTokenizer[startIndex]->text=='(');
    int endIndex=mTokenizer[startIndex]->matchIndex;
    //no args, it must be a function
    if (endIndex-startIndex==1)
        return false;
    int i=startIndex+1; //skip '('
    int endPos = endIndex;
    QString word = "";
    while (i<endPos) {
        QChar ch=mTokenizer[i]->text[0];
        switch(ch.unicode()) {
        // args contains a string/char, can't be a func define
        case '"':
        case '\'':
        case '+':
        case '-':
        case '/':
        case '|':
        case '!':
        case '{':
            return true;
        case '[': // function args like int f[10]
            i=mTokenizer[i]->matchIndex+1;
            if (i<endPos &&
                    (mTokenizer[i]->text=='('
                     || mTokenizer[i]->text=='{')) //lambda
                return true;
            continue;
        }
        if (isDigitChar(ch))
            return true;
        if (isIdentChar(ch)) {
            QString currentText=mTokenizer[i]->text;
            if (mTokenizer[i]->text.endsWith('.'))
                return true;
            if (mTokenizer[i]->text.endsWith("->"))
                return true;
            if (!mCppTypeKeywords.contains(currentText)) {
                if (currentText=="true" || currentText=="false" || currentText=="nullptr" ||
                        currentText=="this")
                    return true;
                if (currentText=="const")
                    return false;

                if (isCppKeyword(currentText))
                    return false;

                PStatement statement =findStatementOf(mCurrentFile,word,getCurrentScope(),true);
                if (statement && isTypeStatement(statement->kind))
                    return false;
            } else {
                return false;
            }

        }
        i++;
    }
    //function with no args

    return false;
}

bool CppParser::isNamedScope(StatementKind kind) const
{
    switch(kind) {
    case StatementKind::skClass:
    case StatementKind::skNamespace:
    case StatementKind::skFunction:
        return true;
    default:
        return false;
    }
}

bool CppParser::isTypeStatement(StatementKind kind) const
{
    switch(kind) {
    case StatementKind::skClass:
    case StatementKind::skTypedef:
    case StatementKind::skEnumClassType:
    case StatementKind::skEnumType:
        return true;
    default:
        return false;
    }
}

void CppParser::updateSerialId()
{
    mSerialId = QString("%1 %2").arg(mParserId).arg(mSerialCount);
}

int CppParser::indexOfNextSemicolon(int index, int endIndex)
{
    if (endIndex<0)
        endIndex=mTokenizer.tokenCount();
    while (index<endIndex) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ';':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexOfNextPeriodOrSemicolon(int index, int endIndex)
{
    if (endIndex<0)
        endIndex=mTokenizer.tokenCount();
    while (index<endIndex) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ';':
        case ',':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexOfNextSemicolonOrLeftBrace(int index)
{
    while (index<mTokenizer.tokenCount()) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ';':
        case '{':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexOfNextColon(int index)
{
    while (index<mTokenizer.tokenCount()) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ':':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexOfNextLeftBrace(int index)
{
    while (index<mTokenizer.tokenCount()) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case '{':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexPassParenthesis(int index)
{
    while (index<mTokenizer.tokenCount()) {
        if (mTokenizer[index]->text=='(') {
            return mTokenizer[index]->matchIndex+1;
        }
        index++;
    }
    return index;
}

int CppParser::indexPassBraces(int index)
{
    while (index<mTokenizer.tokenCount()) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case '{':
            return mTokenizer[index]->matchIndex+1;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

void CppParser::skipNextSemicolon(int index)
{
    mIndex=index;
    while (mIndex<mTokenizer.tokenCount()) {
        switch(mTokenizer[mIndex]->text[0].unicode()) {
        case ';':
            mIndex++;
            return;
        case '{':
            mIndex = mTokenizer[mIndex]->matchIndex+1;
            break;
        case '(':
            mIndex = mTokenizer[mIndex]->matchIndex+1;
            break;
        default:
            mIndex++;
        }
    }
}

//int CppParser::moveToEndOfStatement(int index, bool checkLambda, int endIndex)
//{
//    int startIndex=index;
//    if (endIndex<0)
//        endIndex=mTokenizer.tokenCount();
//    bool stop=false;
//    while (index<endIndex && !stop) {
//        switch(mTokenizer[index]->text[0].unicode()) {
//        case ';':
//            index++;
//            stop=true;
//            break;
//        case '{':
//            //skip '}'
//            index = mTokenizer[index]->matchIndex+1;
//            stop = true;
//            break;
//        case '(':
//            index = mTokenizer[index]->matchIndex+1;
//            break;
//        default:
//            index++;
//        }
//    }
//    if (stop && checkLambda) {
//        while (mTokenizer.lambdasCount()>0 && mTokenizer.indexOfFirstLambda()<index) {
//            int i=mTokenizer.indexOfFirstLambda();
//            mTokenizer.removeFirstLambda();
//            if (i>=startIndex) {
//                handleLambda(i,index);
//            }
//        }
//    }
//    return index;
//}

int CppParser::moveToEndOfStatement(int index, bool checkLambda, int endIndex)
{
    int startIndex=index;
    if (endIndex<0)
        endIndex=mTokenizer.tokenCount();
    if (index>=endIndex)
        return index;
    index--; // compensate for the first loop

    bool skip=true;
    do {
        index++;
        bool stop = false;
        while (index<endIndex && !stop) {
            switch(mTokenizer[index]->text[0].unicode()) {
            case ';':
                stop=true;
                break;
            case '=':
                stop=true;
                break;
            case '}':
                stop=true;
                skip=false; //don't skip the orphan '}' that we found
                break;
            case '{':
                //move to '}'
                index=mTokenizer[index]->matchIndex;
                stop=true;
                break;
            case '(':
                index = mTokenizer[index]->matchIndex+1;
                break;
            default:
                index++;
            }
        }
    } while (index<mTokenizer.tokenCount() && mTokenizer[index]->text=='=');
    if (index<endIndex && checkLambda) {
        while (mTokenizer.lambdasCount()>0 && mTokenizer.indexOfFirstLambda()<index) {
            int i=mTokenizer.indexOfFirstLambda();
            mTokenizer.removeFirstLambda();
            if (i>=startIndex) {
                handleLambda(i,index);
            }
        }
    }
    if (skip)
        index++;
    return index;
}

void CppParser::skipParenthesis(int index)
{
    mIndex=index;
    while (mIndex<mTokenizer.tokenCount()) {
        if (mTokenizer[mIndex]->text=='(') {
            mIndex=mTokenizer[mIndex]->matchIndex+1;
            return;
        }
        mIndex++;
    }
}

int CppParser::skipAssignment(int index, int endIndex)
{
    int startIndex=index;
    bool stop=false;
    while (index<endIndex && !stop) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ';':
        case ',':
        case '{':
            stop=true;
            break;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    if (stop) {
        while (mTokenizer.lambdasCount()>0 && mTokenizer.indexOfFirstLambda()<index) {
            int i=mTokenizer.indexOfFirstLambda();
            mTokenizer.removeFirstLambda();
            if (i>=startIndex) {
                handleLambda(i,index);
            }
        }
    }
    return (index<endIndex)?index:endIndex;
}

QString CppParser::mergeArgs(int startIndex, int endIndex)
{
    QString result;
    for (int i=startIndex;i<=endIndex;i++) {
        if (i>startIndex)
            result+=' ';
        result+=mTokenizer[i]->text;
    }
    return result;
}

void CppParser::parseCommandTypeAndArgs(QString &command, QString &typeSuffix, QString &args)
{
    typeSuffix="";
    while (command.startsWith('*') || command.startsWith('&')) {
        typeSuffix=command.front();
        command=command.mid(1);
    }
    int pos=command.indexOf('[');
    if (pos>=0) {
        args=command.mid(pos);
        command=command.left(pos);
    } else {
        args="";
    }

}

const QSet<QString> &CppParser::projectFiles() const
{
    return mProjectFiles;
}

QList<QString> CppParser::namespaces()
{
    QMutexLocker locker(&mMutex);
    return mNamespaces.keys();
}

ParserLanguage CppParser::language() const
{
    return mLanguage;
}

void CppParser::setLanguage(ParserLanguage newLanguage)
{
    mLanguage = newLanguage;
}



const StatementModel &CppParser::statementList() const
{
    return mStatementList;
}

bool CppParser::parseGlobalHeaders() const
{
    return mParseGlobalHeaders;
}

void CppParser::setParseGlobalHeaders(bool newParseGlobalHeaders)
{
    mParseGlobalHeaders = newParseGlobalHeaders;
}

const QSet<QString> &CppParser::includePaths()
{
    return mPreprocessor.includePaths();
}

const QSet<QString> &CppParser::projectIncludePaths()
{
    return mPreprocessor.projectIncludePaths();
}

bool CppParser::parseLocalHeaders() const
{
    return mParseLocalHeaders;
}

void CppParser::setParseLocalHeaders(bool newParseLocalHeaders)
{
    mParseLocalHeaders = newParseLocalHeaders;
}

const QString &CppParser::serialId() const
{
    return mSerialId;
}

int CppParser::parserId() const
{
    return mParserId;
}

void CppParser::setOnGetFileStream(const GetFileStreamCallBack &newOnGetFileStream)
{
    mOnGetFileStream = newOnGetFileStream;
    mPreprocessor.setOnGetFileStream(newOnGetFileStream);
}

const QSet<QString> &CppParser::filesToScan() const
{
    return mFilesToScan;
}

void CppParser::setFilesToScan(const QSet<QString> &newFilesToScan)
{
    mFilesToScan = newFilesToScan;
}

bool CppParser::enabled() const
{
    return mEnabled;
}

void CppParser::setEnabled(bool newEnabled)
{
    if (mEnabled!=newEnabled) {
        mEnabled = newEnabled;
        if (!mEnabled) {
            resetParser();
        }
    }
}

CppFileParserThread::CppFileParserThread(
        PCppParser parser,
        QString fileName,
        bool inProject,
        bool onlyIfNotParsed,
        bool updateView,
        QObject *parent):QThread(parent),
    mParser(parser),
    mFileName(fileName),
    mInProject(inProject),
    mOnlyIfNotParsed(onlyIfNotParsed),
    mUpdateView(updateView)
{
    connect(this,&QThread::finished,
            this,&QObject::deleteLater);
}

void CppFileParserThread::run()
{
    if (mParser && !mParser->parsing()) {
        mParser->parseFile(mFileName,mInProject,mOnlyIfNotParsed,mUpdateView);
    }
}

CppFileListParserThread::CppFileListParserThread(PCppParser parser,
                                                 bool updateView, QObject *parent):
    QThread(parent),
    mParser(parser),
    mUpdateView(updateView)
{
    connect(this,&QThread::finished,
            this,&QObject::deleteLater);
}

void CppFileListParserThread::run()
{
    if (mParser && !mParser->parsing()) {
        mParser->parseFileList(mUpdateView);
    }
}

void parseFile(PCppParser parser, const QString& fileName, bool inProject, bool onlyIfNotParsed, bool updateView)
{
    if (!parser)
        return;
    if (!parser->enabled())
        return;
    //delete when finished
    CppFileParserThread* thread = new CppFileParserThread(parser,fileName,inProject,onlyIfNotParsed,updateView);
    thread->connect(thread,
                    &QThread::finished,
                    thread,
                    &QThread::deleteLater);
    thread->start();
}

void parseFileList(PCppParser parser, bool updateView)
{
    if (!parser)
        return;
    if (!parser->enabled())
        return;
    //delete when finished
    CppFileListParserThread *thread = new CppFileListParserThread(parser,updateView);
    thread->connect(thread,
                    &QThread::finished,
                    thread,
                    &QThread::deleteLater);
    thread->start();
}
