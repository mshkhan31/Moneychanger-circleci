#ifndef __STABLE_HPP__
#include <core/stable.hpp>
#endif

#include <gui/widgets/dlggetamount.hpp>
#include <ui_dlggetamount.h>

#include <core/moneychanger.hpp>

#include <QMessageBox>
#include <QKeyEvent>

#include <opentxs/opentxs.hpp>

DlgGetAmount::DlgGetAmount(QWidget *parent, QString qstrAcctId, QString qstrInstrumentDefinitionID, QString qstrReason) :
    QDialog(parent),
    m_qstrAcctId(qstrAcctId),
    m_qstrInstrumentDefinitionID(qstrInstrumentDefinitionID),
    m_qstrReason(qstrReason),
    m_lAmount(0),
    m_bValidAmount(false),
    ui(new Ui::DlgGetAmount)
{
    ui->setupUi(this);
    // ----------------------
    ui->labelReason->setText(qstrReason);
    // ----------------------
    std::string str_asset_name  = Moneychanger::It()->OT().Exec().GetAssetType_Name(qstrInstrumentDefinitionID.toStdString());
    QString     qstr_asset_name = QString("<font color=grey>%1</font>").arg(QString::fromStdString(str_asset_name));
    // ----------------------
    ui->labelAsset->setText(qstr_asset_name);
    // ----------------------
    QString     qstrBalance   = Moneychanger::shortAcctBalance(qstrAcctId);
    std::string str_acct_name = Moneychanger::It()->OT().Exec().GetAccountWallet_Name(qstrAcctId.toStdString());
    QString     qstrAcctName  = QString::fromStdString(str_acct_name);
    // ----------------------
    ui->labelBalance->setText(QString("<font color=grey>%1:</font> <big>%2</big>").arg(qstrAcctName).arg(qstrBalance));
    // ----------------------
    std::string str_amount = Moneychanger::It()->OT().Exec().FormatAmount(m_qstrInstrumentDefinitionID.toStdString(), m_lAmount);
    // ----------------------
    ui->lineEdit->setText(QString::fromStdString(str_amount));
    // ----------------------
    this->installEventFilter(this);
}


bool DlgGetAmount::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Escape)
        {
            close(); // This is caught by this same filter.
            return true;
        }
    }
    // standard event processing
    return QDialog::eventFilter(obj, event);
}




void DlgGetAmount::on_buttonBox_rejected()
{
    this->reject();
}

void DlgGetAmount::on_buttonBox_accepted()
{
    on_lineEdit_editingFinished();
    // ------------------------------------
    if (!m_bValidAmount)
    {
        QMessageBox::warning(this, tr("Bad Amount"),
                             tr("Please enter a valid amount."));
        return;
    }
    // ------------------------------------
    std::string str_amount = Moneychanger::It()->OT().Exec().FormatAmount(m_qstrInstrumentDefinitionID.toStdString(), m_lAmount);
    // ------------------------------------
    QMessageBox::StandardButton reply;

    QString qstrQuestion = QString("%1: %2.<br/>%3").arg(tr("The amount entered is")).
                                                 arg(QString::fromStdString(str_amount)).
                                                 arg(tr("Do you wish to proceed?"));

    reply = QMessageBox::question(this, "", qstrQuestion,
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
        this->accept();
    // ------------------------------------
}


//Note: remove this. Used accept instead.
//void DlgGetAmount::closeEvent(QCloseEvent *event)
//{
//    QWidget::closeEvent(event);
//}

void DlgGetAmount::on_lineEdit_editingFinished()
{
    std::string   str_amount;
    m_lAmount   = Moneychanger::It()->OT().Exec().StringToAmount(m_qstrInstrumentDefinitionID.toStdString(), ui->lineEdit->text().toStdString());
    str_amount  = Moneychanger::It()->OT().Exec().FormatAmount  (m_qstrInstrumentDefinitionID.toStdString(), m_lAmount);

    ui->lineEdit->setText(QString::fromStdString(str_amount));
    // --------------------------------
    if (m_lAmount > 0)
        m_bValidAmount = true;
    else
        m_bValidAmount = false;
}


DlgGetAmount::~DlgGetAmount()
{
    delete ui;
}

