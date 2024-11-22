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
#ifndef CPUDIALOG_H
#define CPUDIALOG_H

#include <QDialog>

namespace Ui {
class CPUDialog;
}

class CPUDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CPUDialog(QWidget *parent = nullptr);
    ~CPUDialog();
    void updateInfo();
    void updateButtonStates(bool enable);
public slots:
    void updateDPI(float dpi);
    void setDisassembly(const QString& file, const QString& funcName,const QStringList& lines);
    void resetEditorFont(float dpi);
signals:
    void closed();
private:
    void sendSyntaxCommand();
private:
    Ui::CPUDialog *ui;
    bool mInited;
    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_rdIntel_toggled(bool checked);
    void on_rdATT_toggled(bool checked);
    void on_chkBlendMode_stateChanged(int arg1);
    void on_btnStepOverInstruction_clicked();
    void on_btnStepIntoInstruction_clicked();
    void onUpdateIcons();

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
};

#endif // CPUDIALOG_H
