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
#ifndef REDPANDA_QT_UTILS_H
#define REDPANDA_QT_UTILS_H
#include <type_traits>
#include <utility>
#include <functional>
#include <QString>
#include <QRect>
#include <QStringList>
#include <memory>
#include <QThread>
#include <QProcessEnvironment>

class QByteArray;
class QTextStream;
class QTextCodec;

#define ENCODING_AUTO_DETECT "AUTO"
#define ENCODING_UTF8   "UTF-8"
#define ENCODING_UTF8_BOM "UTF-8 BOM"
#define ENCODING_SYSTEM_DEFAULT   "SYSTEM"
#define ENCODING_ASCII  "ASCII"

enum class FileEndingType {
    Windows,
    Linux,
    Mac
};// Windows: CRLF, UNIX: LF, Mac: CR

class BaseError{
public:
    explicit BaseError(const QString& reason);
    QString reason() const;

protected:
    QString mReason;
};

class IndexOutOfRange:public BaseError {
public:
    explicit IndexOutOfRange(int Index);
};

class FileError: public BaseError {
public:
    explicit FileError(const QString& reason);
};

/* text processing utils */
const QByteArray guessTextEncoding(const QByteArray& text);

bool isTextAllAscii(const QByteArray& text);
bool isTextAllAscii(const QString& text);

bool isNonPrintableAsciiChar(char ch);

using LineProcessFunc =  std::function<void(const QString&)>;

QStringList textToLines(const QString& text);
void textToLines(const QString& text, LineProcessFunc lineFunc);
QString linesToText(const QStringList& lines, const QString& lineBreak="\n");

QList<QByteArray> splitByteArrayToLines(const QByteArray& content);

QString trimRight(const QString& s);
QString trimLeft(const QString& s);

int countLeadingWhitespaceChars(const QString& line);

bool stringIsBlank(const QString& s);

QByteArray toByteArray(const QString& s);
QString fromByteArray(const QByteArray& s);

/* text I/O utils */

QStringList readStreamToLines(QTextStream* stream);
void readStreamToLines(QTextStream* stream, LineProcessFunc lineFunc);

/**
 * @brief readFileToLines
 * @param fileName
 * @param codec
 * @return
 */
QStringList readFileToLines(const QString& fileName, QTextCodec* codec);
QStringList readFileToLines(const QString& fileName);
void readFileToLines(const QString& fileName, QTextCodec* codec, LineProcessFunc lineFunc);

QByteArray readFileToByteArray(const QString& fileName);

void stringsToFile(const QStringList& list, const QString& fileName);
void stringToFile(const QString& str, const QString& fileName);
void createFile(const QString& fileName);

/* File I/O utils */
bool fileExists(const QString& file);
bool fileExists(const QString& dir, const QString& fileName);
bool directoryExists(const QString& file);
bool removeFile(const QString& filename);
void copyFolder(const QString &fromDir, const QString& toDir);
bool copyFile(const QString &fromFile, const QString& toFile, bool overwrite);
bool isInFolder(const QString& folderpath, const QString& filepath);

QString includeTrailingPathDelimiter(const QString& path);
QString excludeTrailingPathDelimiter(const QString& path);
QString changeFileExt(const QString& filename, QString ext);

QString localizePath(const QString& path);

QString extractRelativePath(const QString& base, const QString& dest);
QStringList extractRelativePaths(const QString& base, const QStringList& destList);
QString extractFileName(const QString& fileName);
QString extractFileDir(const QString& fileName);
QString extractFilePath(const QString& filePath);
QString extractAbsoluteFilePath(const QString& filePath);
QString cleanPath(const QString& dirPath);
QString generateAbsolutePath(const QString& dirPath, const QString& relativePath);
QStringList absolutePaths(const QString& dirPath, const QStringList& relativePaths);
QString escapeSpacesInString(const QString& str);


bool isReadOnly(const QString& filename);

int compareFileModifiedTime(const QString& filename1, const QString& filename2);

/* UI utils */
void inflateRect(QRect& rect, int delta);
void inflateRect(QRect& rect, int dx, int dy);

int screenDPI();
void setScreenDPI(int dpi);
float pointToPixel(float point);
float pointToPixel(float point, float dpi);
float pixelToPoint(float pixel);

void decodeKey(int combinedKey, int& key, Qt::KeyboardModifiers& modifiers);


/**
 * from https://github.com/Microsoft/GSL
 **/

template <class F>
class final_action
{
public:
    static_assert(!std::is_reference<F>::value && !std::is_const<F>::value &&
                      !std::is_volatile<F>::value,
                  "Final_action should store its callable by value");

    explicit final_action(F f) noexcept : f_(std::move(f)) {}

    final_action(final_action&& other) noexcept
        : f_(std::move(other.f_)), invoke_(std::exchange(other.invoke_, false))
    {}

    final_action(const final_action&) = delete;
    final_action& operator=(const final_action&) = delete;
    final_action& operator=(final_action&&) = delete;

    ~final_action() noexcept
    {
        if (invoke_) f_();
    }

private:
    F f_;
    bool invoke_{true};
};

template <class F> final_action<typename std::remove_cv<typename std::remove_reference<F>::type>::type>
finally(F&& f) noexcept
{
    return final_action<typename std::remove_cv<typename std::remove_reference<F>::type>::type>(
        std::forward<F>(f));
}
#endif // UTILS_H
