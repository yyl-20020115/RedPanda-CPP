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
#include "systemconsts.h"
#include "utils.h"
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextCodec>

SystemConsts* pSystemConsts;

SystemConsts::SystemConsts(): mDefaultFileFilters()
{
    addDefaultFileFilter(QObject::tr("All files"),"*");
    addDefaultFileFilter(QObject::tr("Dev C++ Project files"),"*.dev");
    addDefaultFileFilter(QObject::tr("C files"),"*.c");
    addDefaultFileFilter(QObject::tr("C++ files"),"*.cpp *.cc *.cxx");
    addDefaultFileFilter(QObject::tr("Header files"),"*.h *.hh");

    addFileFilter(mIconFileFilters, QObject::tr("Icon files"), "*.ico");

    mCodecNames.append(ENCODING_AUTO_DETECT);
    mCodecNames.append(ENCODING_SYSTEM_DEFAULT);
    mCodecNames.append(ENCODING_UTF8);
    mCodecNames.append(ENCODING_UTF8_BOM);
    QStringList codecNames;
    QSet<QByteArray> codecAlias;
    codecAlias.insert("system");
    codecAlias.insert("utf-8");

    foreach (const QByteArray& name, QTextCodec::availableCodecs()){
        QByteArray lname = name.toLower();
        if (lname.startsWith("cp"))
            continue;
        if (codecAlias.contains(lname))
            continue;
        codecNames.append(lname);
        codecAlias.insert(lname);
        QTextCodec* codec = QTextCodec::codecForName(name);
        if (codec) {
            foreach (const QByteArray& alias, codec->aliases()) {
                codecAlias.insert(alias.toLower());
            }
        }
    }
    std::sort(codecNames.begin(),codecNames.end());
    mCodecNames.append(codecNames);

    mDefaultFileNameFilters.append("*.c");
    mDefaultFileNameFilters.append("*.cpp");
    mDefaultFileNameFilters.append("*.cc");
    mDefaultFileNameFilters.append("*.C");
    mDefaultFileNameFilters.append("*.cxx");
    mDefaultFileNameFilters.append("*.cxx");
    mDefaultFileNameFilters.append("*.h");
    mDefaultFileNameFilters.append("*.hpp");
    mDefaultFileNameFilters.append("*.hxx");
    mDefaultFileNameFilters.append(".gitignore");
    mDefaultFileNameFilters.append("*.vs");
    mDefaultFileNameFilters.append("*.fs");
    mDefaultFileNameFilters.append("*.txt");
    mDefaultFileNameFilters.append("*.in");
    mDefaultFileNameFilters.append("*.out");
    mDefaultFileNameFilters.append("*.dat");
    mDefaultFileNameFilters.append("*.md");
    mDefaultFileNameFilters.append("*.dev");
}

const QStringList &SystemConsts::defaultFileFilters() const noexcept
{
    return mDefaultFileFilters;
}

const QString &SystemConsts::defaultCFileFilter() const noexcept
{
    return mDefaultFileFilters[2];
}

const QString &SystemConsts::defaultCPPFileFilter() const noexcept
{
    return mDefaultFileFilters[3];
}

const QString &SystemConsts::defaultAllFileFilter() const noexcept
{
    return mDefaultFileFilters[0];
}

void SystemConsts::addDefaultFileFilter(const QString &name, const QString &fileExtensions)
{
    addFileFilter(mDefaultFileFilters,name,fileExtensions);
}

void SystemConsts::addFileFilter(QStringList& filters, const QString &name, const QString &fileExtensions)
{
    filters.append(name+ " (" + fileExtensions+")");
}

const QStringList &SystemConsts::defaultFileNameFilters() const
{
    return mDefaultFileNameFilters;
}

const QStringList &SystemConsts::codecNames() const
{
    return mCodecNames;
}

const QStringList &SystemConsts::iconFileFilters() const
{
    return mIconFileFilters;
}

const QString &SystemConsts::iconFileFilter() const
{
    return mIconFileFilters[0];
}

void SystemConsts::setIconFileFilters(const QStringList &newIconFileFilters)
{
    mIconFileFilters = newIconFileFilters;
}
