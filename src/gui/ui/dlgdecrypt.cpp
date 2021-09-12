#ifndef __STABLE_HPP__
#include <core/stable.hpp>
#endif

#include "dlgdecrypt.hpp"
#include "ui_dlgdecrypt.h"

#include <gui/ui/dlgexportedtopass.hpp>

#include <core/handlers/contacthandler.hpp>

#include <QClipboard>
#include <QMessageBox>
#include <QDebug>

#include <core/moneychanger.hpp>
#include <core/handlers/focuser.h>

#include <opentxs/opentxs.hpp>

DlgDecrypt::DlgDecrypt(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDecrypt),
    already_init(false)
{
    ui->setupUi(this);

    this->installEventFilter(this);
}

DlgDecrypt::~DlgDecrypt()
{
    delete ui;
}



bool DlgDecrypt::eventFilter(QObject *obj, QEvent *event)
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


void DlgDecrypt::on_pushButtonClipboard_clicked()
{
    // Get text from the clipboard, and add it to the plainTextEdit widget
    //
    QClipboard *clipboard = QApplication::clipboard();

    if (NULL != clipboard)
    {
        QString qstrClipText = clipboard->text();

        if (!qstrClipText.isEmpty())
            ui->plainTextEdit->insertPlainText(qstrClipText);
    }
}

void DlgDecrypt::on_pushButtonDecrypt_clicked()
{
    bool bSuccessDecrypting         = false;
    bool bSuccessVerifying          = false;
    bool bSuccessDecodingBase64     = false;
    bool bSuccessDecodingArmor      = false;
    bool bSuccessDecodingSignedFile = false;

    QString qstrNymWhoDecrypted;
    QString qstrNymWhoVerified;

    QString qstrText = ui->plainTextEdit->toPlainText().trimmed();
    // --------------------------------
    if (qstrText.isEmpty())
    {
        // pop up a message box warning that the input text is empty.
        //
        QMessageBox::warning(this, tr("Ciphertext is Empty"),
                             tr("Please paste something to be decrypted/verified."));
        return;
    }
    // --------------------------------
    else //qstrText not empty.
    {
        std::string     str_input(qstrText.toStdString());
        opentxs::String strInput (str_input.c_str());

        if (strInput.Exists())
        {
            if (strInput.Contains("-----BEGIN OT ARMORED ENVELOPE-----"))
            {
                opentxs::OTEnvelope theEnvelope;
                opentxs::Armored armor;
                opentxs::Armored::LoadFromString(armor, strInput);

                if (theEnvelope.SetCiphertext(armor))
                {
                    opentxs::String strOutput;
                    // -------------------------
                    // First we'll try the default nym, if one is available.
                    //
                    QString qstrTempID = Moneychanger::It()->get_default_nym_id();

                    if (!qstrTempID.isEmpty()) // Default Nym IS available.
                    {
                        std::string  str_nym    (qstrTempID.toStdString());
                        opentxs::String     strNym     (str_nym.c_str());
                        auto nym_id     = opentxs::Identifier::Factory(strNym);

                        if (!nym_id->empty())
                        {
                            opentxs::OTPasswordData thePWData("Recipient passphrase");

                            std::shared_ptr<const opentxs::Nym> pNym = Moneychanger::It()->OT().Wallet().Nym(nym_id);
                            if (false != bool(pNym))
                            {
                                if (theEnvelope.Open(*pNym, strOutput) && strOutput.Exists())
                                {
                                    bSuccessDecrypting = true;
                                    qstrNymWhoDecrypted = qstrTempID;

                                    strInput  = strOutput;
                                    str_input = strInput.Get();
                                    qstrText  = QString::fromStdString(str_input);
                                }
                            }
                        }
                    }
                    // ------------
                    if (!bSuccessDecrypting) // Default nym is NOT available. Okay let's loop through all the Nyms in the wallet then, and try then all...
                    {
                        const int32_t nym_count = Moneychanger::It()->OT().Exec().GetNymCount();
                        // -----------------------------------------------
                        for (int32_t ii = 0; ii < nym_count; ++ii)
                        {
                            //Get OT Nym ID
                            QString OT_nym_id = QString::fromStdString(Moneychanger::It()->OT().Exec().GetNym_ID(ii));

                            if (!OT_nym_id.isEmpty())
                            {
                                std::string         str_nym    (OT_nym_id.toStdString());
                                opentxs::String     strNym     (str_nym.c_str());
                                auto nym_id     = opentxs::Identifier::Factory(strNym);

                                if (!nym_id->empty())
                                {
                                    opentxs::OTPasswordData thePWData("Recipient passphrase");

                                    std::shared_ptr<const opentxs::Nym> pNym = Moneychanger::It()->OT().Wallet().Nym(nym_id);
                                    if (false != bool(pNym))
                                    {
                                        // Okay there is a private key available for this Nym, so let's
                                        // try to open the envelope using it.
                                        //
                                        if (theEnvelope.Open(*pNym, strOutput) && strOutput.Exists())
                                        {
                                            bSuccessDecrypting = true;
                                            qstrNymWhoDecrypted = OT_nym_id;

                                            strInput  = strOutput;
                                            str_input = strInput.Get();
                                            qstrText  = QString::fromStdString(str_input);

                                            break;
                                        }
                                    }
                                }
                                // ------------
                            }
                        } // for
                    } // else default nym not available.
                    // -----------------------
                    if (!bSuccessDecrypting)
                    {
                        QMessageBox::warning(this, tr("Failed Decrypting"),
                                             tr("None of the identities in your wallet (including your default identity, "
                                                "if applicable) were able to open this message."));
                        return;

                    }
                    // -----------------------
                } // if (theEnvelope.SetFromBookendedString(strInput))

            } // if (strInput.Contains("-----BEGIN OT ARMORED ENVELOPE-----"))
            // --------------------------------------------
            // This call to DecodeIfArmored is what handles the: "-----BEGIN OT ARMORED ... -----"

            if (strInput.DecodeIfArmored()) // bEscapedIsAllowed=true by default.
            {
                std::string str_decoded(strInput.Get());
                QString qstrDecoded(QString::fromStdString(str_decoded));

                if (!qstrDecoded.isEmpty()) {
                    bSuccessDecodingArmor = true;
                    qstrText = qstrDecoded;
                    str_input = str_decoded;
                }
                // -----------------------------------
                // At this point, we know it's been decrypted, if applicable, and it's been
                // de-armored, if applicable. So now we check to see if it's a signed file.
                //
                if (strInput.Contains("-----BEGIN SIGNED FILE-----"))
                {
                    auto theSignedFile{Moneychanger::It()->OT().Factory().SignedFile()};
                    
                    OT_ASSERT(false != bool(theSignedFile));

                    if (theSignedFile->LoadContractFromString(strInput))
                    {
                        opentxs::String strSignerNymID = theSignedFile->GetSignerNymID();
                        std::string str_signer_nym(strSignerNymID.Get());
                        QString qstrSignerNym(QString::fromStdString(str_signer_nym));

                        if (!str_signer_nym.empty())
                        {
                            opentxs::OTPasswordData thePWData("Sometimes need to load private part of nym in order to use its public key. (Fix that!)");
                            auto id_signer_nym = opentxs::Identifier::Factory(strSignerNymID);
                            std::shared_ptr<const opentxs::Nym> pNym = Moneychanger::It()->OT().Wallet().Nym(id_signer_nym);
                            if (false != bool(pNym))
                            {
                                if (theSignedFile->VerifySignature(*pNym, &thePWData))
                                {
                                    bSuccessVerifying = true;
                                    qstrNymWhoVerified = qstrSignerNym;

                                    opentxs::String strContents = theSignedFile->GetFilePayload();

                                    if (strContents.Exists())
                                    {
                                        bSuccessDecodingSignedFile = true;
                                        strInput  = strContents;
                                        str_input = strInput.Get();
                                        qstrText  = QString::fromStdString(str_input);
                                    }
                                } // signature verified
                            } // pNym exists
                        } // if str_signer_nym exists
                    } // signed file: load contract from string.
                } // "BEGIN SIGNED FILE"
            } // Decode If Armored.
            // -----------------------------------------------

            if (Moneychanger::is_base64(qstrText))
            {
                opentxs::Armored ascText{str_input.c_str()};
                ascText.GetString(strInput);
                str_input = strInput.Get();
                qstrText  = QString::fromStdString(str_input);

                bSuccessDecodingBase64 = true;
            }

            // -----------------------------------------------
            // if qstrText still contains something, pop up a dialog to display the result to the user.
            //
            if (!qstrText.isEmpty())
            {
                QString qstrNymWhoDecryptedName(""), qstrNymWhoVerifiedName("");

                if (!qstrNymWhoDecrypted.isEmpty())
                    qstrNymWhoDecryptedName = QString::fromStdString(Moneychanger::It()->OT().Exec().GetNym_Name(qstrNymWhoDecrypted.toStdString()));
                if (!qstrNymWhoVerified.isEmpty())
                    qstrNymWhoVerifiedName = QString::fromStdString(Moneychanger::It()->OT().Exec().GetNym_Name(qstrNymWhoVerified.toStdString()));
                // -------------------------------
                if (qstrNymWhoDecryptedName.isEmpty())
                    qstrNymWhoDecryptedName = qstrNymWhoDecrypted;
                else if (qstrNymWhoDecryptedName != qstrNymWhoDecrypted)
                    qstrNymWhoDecryptedName += QString(" (%1)").arg(qstrNymWhoDecrypted);
                // -------------------------------
                if (qstrNymWhoVerifiedName.isEmpty())
                    qstrNymWhoVerifiedName = qstrNymWhoVerified;
                else if (qstrNymWhoVerifiedName != qstrNymWhoVerified)
                    qstrNymWhoVerifiedName += QString(" (%1)").arg(qstrNymWhoVerified);
                // -------------------------------
                QString qstrType(QString("%1:").arg(tr("Output")));

                if (bSuccessDecodingArmor) {
                    qstrType = QString("%1:").arg(tr("Decoded armored text"));
                }
                if (bSuccessDecodingBase64) {
                    qstrType = QString("%1:").arg(tr("Decoded base64 text"));
                }
                if (bSuccessDecodingSignedFile) {
                    qstrType = QString("%1:").arg(tr("Decoded signed file"));
                }
                // -------------------------------
                if (bSuccessVerifying)
                {
                    qstrType = QString("%1: %2").arg(tr("Verified signature by Nym")).arg(qstrNymWhoVerifiedName);
                }
                // -----------
                if (bSuccessDecrypting)
                {
                    if (bSuccessVerifying)
                        qstrType = QString("%1: %2\n%3: %4").arg(tr("Decrypted using Nym")).arg(qstrNymWhoDecryptedName).arg(tr("Verified signature by Nym")).arg(qstrNymWhoVerifiedName);
                    else
                        qstrType = QString("%1: %2").arg(tr("Decrypted using Nym")).arg(qstrNymWhoDecryptedName);
                }
                // -----------
                QString qstrSubTitle(tr("Be sure to copy it somewhere before closing this dialog."));
                // -----------
                // Pop up the result dialog.
                //
                DlgExportedToPass dlgExported(this, qstrText,
                                              qstrType,
                                              qstrSubTitle, false);
                dlgExported.exec();
            } // if (!qstrText.isEmpty())
        } // if strInput
    } //qstrText not empty
}


void DlgDecrypt::dialog()
{
    if (!already_init)
    {
        Focuser f(this);
        f.show();
        f.focus();

        already_init = true;
    }
}


