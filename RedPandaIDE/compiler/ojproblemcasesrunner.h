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
#ifndef OJPROBLEMCASESRUNNER_H
#define OJPROBLEMCASESRUNNER_H

#include "runner.h"
#include <QVector>
#include "../problems/ojproblemset.h"

class OJProblemCasesRunner : public Runner
{
    Q_OBJECT
public:
    explicit OJProblemCasesRunner(const QString& filename, const QString& arguments, const QString& workDir,
                                  const QVector<POJProblemCase>& problemCases, QObject *parent = nullptr);
    explicit OJProblemCasesRunner(const QString& filename, const QString& arguments, const QString& workDir,
                                  POJProblemCase problemCase, QObject *parent = nullptr);
    //max size of output buffer
    int bufferSize() const;
    void setBufferSize(int newBufferSize);

    //max time (in milliseconds) waiting to flush output buffer
    int outputRefreshTime() const;
    void setOutputRefreshTime(int newOutputRefreshTime);

    int waitForFinishTime() const;
    void setWaitForFinishTime(int newWaitForFinishTime);

    int execTimeout() const;
    void setExecTimeout(int newExecTimeout);

signals:
    void caseStarted(const QString &caseId, int current, int total);
    void caseFinished(const QString &caseId, int current, int total);
    void newOutputGetted(const QString &caseId, const QString &newOutputLine);
    void resetOutput(const QString &caseId, const QString &newOutputLine);
private:
    void runCase(int index, POJProblemCase problemCase);
private:
    QVector<POJProblemCase> mProblemCases;

    // QThread interface
protected:
    void run() override;
private:
    int mBufferSize;
    int mOutputRefreshTime;
    int mExecTimeout;
};

#endif // OJPROBLEMCASESRUNNER_H
