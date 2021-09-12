#ifndef __STABLE_HPP__
#include <core/stable.hpp>
#endif

#include <gui/widgets/assetdetails.hpp>
#include <ui_assetdetails.h>

#include <gui/widgets/dlgchooser.hpp>
#include <gui/widgets/wizardaddcontract.hpp>
#include <gui/widgets/overridecursor.hpp>

#include <core/moneychanger.hpp>

#include <opentxs/opentxs.hpp>

#include <QPlainTextEdit>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QClipboard>


MTAssetDetails::MTAssetDetails(QWidget *parent, MTDetailEdit & theOwner) :
    MTEditDetails(parent, theOwner),
    ui(new Ui::MTAssetDetails)
{
    ui->setupUi(this);
    this->setContentsMargins(0, 0, 0, 0);
//  this->installEventFilter(this); // NOTE: Successfully tested theory that the base class has already installed this.

    ui->lineEditID   ->setStyleSheet("QLineEdit { background-color: lightgray }");
    ui->lineEditNymID->setStyleSheet("QLineEdit { background-color: lightgray }");
    // ----------------------------------
    // Note: This is a placekeeper, so later on I can just erase
    // the widget at 0 and replace it with the real header widget.
    //
    m_pHeaderWidget  = new QWidget;
    ui->verticalLayout->insertWidget(0, m_pHeaderWidget);
    // ----------------------------------
    ui->plainTextEditDetails->setStyleSheet("QPlainTextEdit { background-color: lightgray }");
    ui->plainTextEditDetails->setReadOnly(true);
}

MTAssetDetails::~MTAssetDetails()
{
    delete ui;
}


// Issue a currency onto a notary based on the currently-selected contract.
//
void MTAssetDetails::on_pushButton_clicked()
{
//    if (m_pPlainTextEdit)
//        qstrContents = m_pPlainTextEdit->toPlainText();
    // --------------------------
    if (Moneychanger::It()->OT().Exec().GetServerCount() <= 0)
    {
        QMessageBox::information(this, tr(MONEYCHANGER_APP_NAME),
                                 tr("There are no server contracts in this wallet. "
                                    "(The notary is the server.) Please add a server contract (aka notary "
                                    "contract) to your wallet, and then come back here and try to issue an asset onto that server."));
        return;
    }
    // --------------------------
    QString qstrAssetID = ui->lineEditID->text();
    QString qstrNymID;

    if (!qstrAssetID.isEmpty())
    {
        QString qstrContents = QString::fromStdString(Moneychanger::It()->OT().Exec().GetAssetType_Contract(qstrAssetID.toStdString()));
        opentxs::proto::UnitDefinition contractProto =
            opentxs::proto::StringToProto<opentxs::proto::UnitDefinition>
                (opentxs::String(qstrContents.toStdString()));

        if (!qstrContents.isEmpty())
        {
            // First we get the "signer nym" ID from the asset contract.
            std::string str_signer_nym = contractProto.nymid();

            if (!str_signer_nym.empty())
            {
                qstrNymID = QString::fromStdString(str_signer_nym);
                // --------------------------
                // Then we see if the local wallet actually contains the private key
                // for that Nym.
                if (Moneychanger::It()->OT().Exec().VerifyUserPrivateKey(str_signer_nym))
                {
                    // Ideally at this point, we will already have some way of differentiating
                    // between the notaries where the assets have, and have not, already been
                    // issued.
                    // This is really OT client's responsibility to track this by keeping a copy
                    // of it's issuing receipt. (The notary should keep this receipt as well.)
                    // The notary needs this receipt so it can prove later that the currency
                    // really was issued onto his notary (and how many units the issuer authorized.)
                    // Whereas the client should keep these receipts so he can know which ones are
                    // issued on which servers.
                    //
                    // In the meantime, the best we can do is ask the servers to tell us what they
                    // know about certain currencies. It's a bit slower but it will work.
                    //
                    // But I'm not coding that tonight. Instead, I'll just ask the server and
                    // try it -- whether it succeeds or not I'll just pop up a message box.
                    //

                    // TODO: Need a dialog to pop up here

                    // We already have the asset ID, and the Nym ID.
                    // We'll let the user choose the server.
                    // Then check to see if that Nym is registered on that server.
                    // Then actually call the issue message.
                    //
                    QString qstr_default_id = Moneychanger::It()->get_default_notary_id();
                    // -------------------------------------------
                    QString qstr_current_id = qstr_default_id;
                    // -------------------------------------------
                    if (qstr_current_id.isEmpty())
                        //&& (Moneychanger::It()->OT().Exec().GetServerCount() > 0)) // Already checked at the top of this function.
                        qstr_current_id = QString::fromStdString(Moneychanger::It()->OT().Exec().GetServer_ID(0));
                    // -------------------------------------------
                    // Select from Servers in local wallet.
                    //
                    DlgChooser theChooser(this);
                    // -----------------------------------------------
                    mapIDName & the_map = theChooser.m_map;

                    bool bFoundDefault = false;
                    // -----------------------------------------------
                    const int32_t the_count = Moneychanger::It()->OT().Exec().GetServerCount();
                    // -----------------------------------------------
                    for (int32_t ii = 0; ii < the_count; ++ii)
                    {
                        QString OT_id = QString::fromStdString(Moneychanger::It()->OT().Exec().GetServer_ID(ii));
                        QString OT_name("");
                        // -----------------------------------------------
                        if (!OT_id.isEmpty())
                        {
                            if (!qstr_current_id.isEmpty() && (0 == OT_id.compare(qstr_current_id)))
                                bFoundDefault = true;
                            // -----------------------------------------------
                            OT_name = QString::fromStdString(Moneychanger::It()->OT().Exec().GetServer_Name(OT_id.toStdString()));
                            // -----------------------------------------------
                            the_map.insert(OT_id, OT_name);
                        }
                     }
                    // -----------------------------------------------
                    if (bFoundDefault)
                        theChooser.SetPreSelected(qstr_current_id);
                    // -----------------------------------------------
                    theChooser.setWindowTitle(tr("Select the Notary"));
                    // -----------------------------------------------
                    if (theChooser.exec() == QDialog::Accepted)
                    {
                        if (!theChooser.m_qstrCurrentID.isEmpty())
                        {
//                            setField("NotaryID",   theChooser.m_qstrCurrentID);
//                            setField("ServerName", theChooser.m_qstrCurrentName);
                            // -----------------------------------------
                            QString qstrNotaryID = theChooser.m_qstrCurrentID;

                            // Then check to see if that Nym is registered on that server.
                            //
                            bool bIsRegiseredAtServer =
                                    Moneychanger::It()->OT().Exec().IsNym_RegisteredAtServer(qstrNymID   .toStdString(),
                                                                                        qstrNotaryID.toStdString());
                            if (!bIsRegiseredAtServer)
                            {
                                // If the Nym's not registered at the server, then register him first.
                                //
                                int32_t nSuccess = 0;
                                {
                                    MTSpinner theSpinner;

                                    auto strResponse = Moneychanger::It()->OT().Sync().RegisterNym(opentxs::Identifier::Factory(qstrNymID.toStdString()),
                                                                                                   opentxs::Identifier::Factory(qstrNotaryID.toStdString()), true);

                                    if (false == strResponse->empty()) {
                                        nSuccess = 1;
                                    } else {
                                        nSuccess = 0;
                                    }
                                }
                                // -1 is error,
                                //  0 is reply received: failure
                                //  1 is reply received: success
                                //
                                switch (nSuccess)
                                {
                                case (1):
                                    {
                                        bIsRegiseredAtServer = true;
                                        MTContactHandler::getInstance()->NotifyOfNymServerPair(qstrNymID, qstrNotaryID);
                                        break; // SUCCESS
                                    }
                                case (0):
                                    {
                                        QMessageBox::warning(this, tr("Failed Registration"),
                                            tr("Failed while trying to register Nym at Server."));
                                        break;
                                    }
                                default:
                                    {
                                        QMessageBox::warning(this, tr("Error in Registration"),
                                            tr("Error while trying to register Nym at Server."));
                                        break;
                                    }
                                } // switch
                                // --------------------------
                                if (1 != nSuccess)
                                {
                                    Moneychanger::It()->HasUsageCredits(qstrNotaryID, qstrNymID);
                                    return;
                                }
                            } // is registered at server.

                            // ----------------------------------------
                            // Then actually call the issue message.
                            //
                            if (bIsRegiseredAtServer)
                            {
                                const auto notaryID = opentxs::Identifier::Factory(qstrNotaryID.toStdString()),
                                        nymID = opentxs::Identifier::Factory(qstrNymID.toStdString()),
                                        assetID = opentxs::Identifier::Factory(qstrAssetID.toStdString());
                                // -----------------------------------
                                {
                                    bool bSuccess = false;
                                    {
                                        MTSpinner theSpinner;

                                        auto action = Moneychanger::It()->OT().ServerAction().DownloadContract(nymID, notaryID, assetID);
                                        const std::string str_reply = action->Run();
                                        const int32_t     nResult   = opentxs::VerifyMessageSuccess(str_reply);

                                        bSuccess = (1 == nResult);
                                    }
                                    // -----------------------------------
                                    if (bSuccess)
                                    {
                                        QMessageBox::information(this, tr(MONEYCHANGER_APP_NAME),
                                                                 tr("Apparently this asset is already registered on the selected notary. "
                                                                    "(I just asked the notary.)"));
                                        return;
                                    }
                                }
                                // -----------------------------------
                                {
                                    std::string str_issuer_acct;

                                    bool bSuccess = false;
                                    {
                                        MTSpinner theSpinner;

                                        auto action = Moneychanger::It()->OT().ServerAction().IssueUnitDefinition(nymID, notaryID,
                                        		contractProto);
                                        const std::string str_reply = action->Run();
                                        const int32_t     nResult   = opentxs::VerifyMessageSuccess(str_reply);

                                        bSuccess = (1 == nResult);

                                        if (bSuccess)
                                            str_issuer_acct = Moneychanger::It()->OT().Exec().Message_GetNewIssuerAcctID(str_reply);
                                    }
                                    // -----------------------------------
                                    if (!bSuccess)
                                    {
                                        QMessageBox::information(this, tr(MONEYCHANGER_APP_NAME),
                                                                 QString(tr("%1 '%2' %3 '%4'. %5")).
                                                                 arg(tr("Failed to register")).
                                                                 arg(ui->lineEditName->text()).
                                                                 arg(tr("on notary")).
                                                                 arg(theChooser.m_qstrCurrentName).
                                                                 arg(tr("<br>Perhaps it is already registered there? Alternately, the notary "
                                                                        " admin may have the permissions turned off.")));

                                        Moneychanger::It()->HasUsageCredits(qstrNotaryID, qstrNymID);
                                        return;
                                    }

                                    QString qstrNewIssuerAcct(QString::fromStdString(str_issuer_acct));

                                    QMessageBox::information(this, tr(MONEYCHANGER_APP_NAME),
                                                             QString(tr("%1 '%2' %3 '%4'. %5: %6")).
                                                             arg(tr("Successfully registered")).
                                                             arg(ui->lineEditName->text()).
                                                             arg(tr("on notary")).
                                                             arg(theChooser.m_qstrCurrentName).
                                                             arg(tr("Your new issuer account was created with the ID")).
                                                             arg(qstrNewIssuerAcct));

                                    QString qstrAcctNewName(tr("New Issuer Account"));
                                    Moneychanger::It()->OT().Exec().SetAccountWallet_Name(str_issuer_acct, str_signer_nym,
                                                                                    qstrAcctNewName.toStdString());
                                    emit newAccountAdded(qstrNewIssuerAcct);
                                    return;
                                }
                                // -----------------------------------
                            }
                        }
                    }
                }
                else // There's no private key in this wallet, for this asset contract.
                {
                    QMessageBox::information(this, tr(MONEYCHANGER_APP_NAME),
                                             tr("Sorry, but only the Nym who signed that asset contract can register it onto a notary."));
                    return;
                }
            }
        }
    }
}


void MTAssetDetails::on_toolButtonAsset_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();

    if (NULL != clipboard)
    {
        clipboard->setText(ui->lineEditID->text());

        QMessageBox::information(this, tr("ID copied"), QString("%1:<br/>%2").
                                 arg(tr("Copied Asset ID to the clipboard")).
                                 arg(ui->lineEditID->text()));
    }
}

void MTAssetDetails::on_toolButtonSigner_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();

    if (NULL != clipboard)
    {
        clipboard->setText(ui->lineEditNymID->text());

        QMessageBox::information(this, tr("ID copied"), QString("%1:<br/>%2").
                                 arg(tr("Copied Signer Nym ID to the clipboard")).
                                 arg(ui->lineEditNymID->text()));
    }
}

// ----------------------------------
//virtual
int MTAssetDetails::GetCustomTabCount()
{
    return 1;
}
// ----------------------------------
//virtual
QWidget * MTAssetDetails::CreateCustomTab(int nTab)
{
    const int nCustomTabCount = this->GetCustomTabCount();
    // -----------------------------
    if ((nTab < 0) || (nTab >= nCustomTabCount))
        return NULL; // out of bounds.
    // -----------------------------
    QWidget * pReturnValue = NULL;
    // -----------------------------
    switch (nTab)
    {
    case 0:
    {
        if (m_pPlainTextEdit)
        {
            m_pPlainTextEdit->setParent(NULL);
            m_pPlainTextEdit->disconnect();
            m_pPlainTextEdit->deleteLater();

            m_pPlainTextEdit = NULL;
        }

        m_pPlainTextEdit = new QPlainTextEdit;

        m_pPlainTextEdit->setReadOnly(true);
        m_pPlainTextEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        // -------------------------------
        QVBoxLayout * pvBox = new QVBoxLayout;

        QLabel * pLabelContents = new QLabel(tr("Asset Contract:"));

        pvBox->setAlignment(Qt::AlignTop);
        pvBox->addWidget   (pLabelContents);
        pvBox->addWidget   (m_pPlainTextEdit);
        // -------------------------------
        pReturnValue = new QWidget;
        pReturnValue->setContentsMargins(0, 0, 0, 0);
        pReturnValue->setLayout(pvBox);
    }
        break;

    default:
        qDebug() << QString("Unexpected: MTAssetDetails::CreateCustomTab was called with bad index: %1").arg(nTab);
        return NULL;
    }
    // -----------------------------
    return pReturnValue;
}
// ---------------------------------
//virtual
QString  MTAssetDetails::GetCustomTabName(int nTab)
{
    const int nCustomTabCount = this->GetCustomTabCount();
    // -----------------------------
    if ((nTab < 0) || (nTab >= nCustomTabCount))
        return QString(""); // out of bounds.
    // -----------------------------
    QString qstrReturnValue("");
    // -----------------------------
    switch (nTab)
    {
    case 0:  qstrReturnValue = "Asset Contract";  break;

    default:
        qDebug() << QString("Unexpected: MTAssetDetails::GetCustomTabName was called with bad index: %1").arg(nTab);
        return QString("");
    }
    // -----------------------------
    return qstrReturnValue;
}
// ------------------------------------------------------


void MTAssetDetails::FavorLeftSideForIDs()
{
    if (NULL != ui)
    {
        ui->lineEditID   ->home(false);
        ui->lineEditName ->home(false);
        ui->lineEditNymID->home(false);
    }
}

void MTAssetDetails::ClearContents()
{
    ui->lineEditID   ->setText("");
    ui->lineEditName ->setText("");
    ui->lineEditNymID->setText("");

    if (m_pPlainTextEdit)
        m_pPlainTextEdit->setPlainText("");

    ui->plainTextEditDetails->setPlainText("");
}

// ------------------------------------------------------

bool MTAssetDetails::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize)
    {
        // This insures that the left-most part of the IDs and Names
        // remains visible during all resize events.
        //
        FavorLeftSideForIDs();
    }
//    else
//    {
        // standard event processing
//        return QWidget::eventFilter(obj, event);
        return MTEditDetails::eventFilter(obj, event);

        // NOTE: Since the base class has definitely already installed this
        // function as the event filter, I must assume that this version
        // is overriding the version in the base class.
        //
        // Therefore I call the base class version here, since as it's overridden,
        // I don't expect it will otherwise ever get called.
//    }
}


// ------------------------------------------------------


//virtual
void MTAssetDetails::DeleteButtonClicked()
{
    if (!m_pOwner->m_qstrCurrentID.isEmpty())
    {
        // ----------------------------------------------------
        bool bCanRemove = Moneychanger::It()->OT().Exec().Wallet_CanRemoveAssetType(m_pOwner->m_qstrCurrentID.toStdString());

        if (!bCanRemove)
        {
            QMessageBox::warning(this, tr(MONEYCHANGER_APP_NAME),
                                 tr("This asset contract cannot be removed, because there are still accounts in the wallet that are using it. Please delete those accounts first."));
            return;
        }
        // ----------------------------------------------------
        QMessageBox::StandardButton reply;

        reply = QMessageBox::question(this, "", tr("Are you sure you want to delete this Asset Contract?"),
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            bool bSuccess = Moneychanger::It()->OT().Exec().Wallet_RemoveAssetType(m_pOwner->m_qstrCurrentID.toStdString());

            if (bSuccess)
            {
                m_pOwner->m_map.remove(m_pOwner->m_qstrCurrentID);
                // ------------------------------------------------
                emit assetsChanged();
                // ------------------------------------------------
            }
            else
                QMessageBox::warning(this, tr("Failure Removing Asset Contract"),
                                     tr("Failed trying to remove this Asset Contract."));
        }
    }
    // ----------------------------------------------------
}


// ------------------------------------------------------

void MTAssetDetails::DownloadedURL()
{
    QString qstrContents;

    if (m_pDownloader)
        qstrContents = m_pDownloader->downloadedData();
    // ----------------------------
    if (qstrContents.isEmpty())
    {
        QMessageBox::warning(this, tr("File at URL Was Empty"),
                             tr("File at specified URL was apparently empty"));
        return;
    }
    // ----------------------------
    ImportContract(qstrContents);
}

// ------------------------------------------------------

void MTAssetDetails::ImportContract(QString qstrContents)
{
    if (qstrContents.isEmpty())
    {
        QMessageBox::warning(this, tr("Contract is Empty"),
            tr("Failed importing: contract is empty."));
        return;
    }
    // ------------------------------------------------------
    QString qstrContractID =
        QString::fromStdString(Moneychanger::It()->OT().Exec().
            AddUnitDefinition(qstrContents.toStdString()));

    if (qstrContractID.isEmpty())
    {
        QMessageBox::warning(this, tr("Failed Importing Asset Contract"),
                             tr("Failed trying to import contract. Is it already in the wallet?"));
        return;
    }
    // ------------------------------------------------------
    else
    {
        // -----------------------------------------------
        QString qstrContractName =
            QString::fromStdString(Moneychanger::It()->OT().Exec().
                GetAssetType_Name(qstrContractID.toStdString()));
        m_pOwner->m_map.insert(qstrContractID, qstrContractName);
        m_pOwner->SetPreSelected(qstrContractID);
        // ------------------------------------------------
        emit newAssetAdded(qstrContractID);
        // ------------------------------------------------
    } // if (!qstrContractID.isEmpty())
}

// ------------------------------------------------------

//virtual
void MTAssetDetails::AddButtonClicked()
{
    MTWizardAddContract theWizard(this);

    theWizard.setAssetMode();

    theWizard.setWindowTitle(tr("Add Asset Contract"));

    QString qstrDefaultValue("https://raw.github.com/FellowTraveler/Open-Transactions/master/sample-data/sample-contracts/btc.otc");
    QVariant varDefault(qstrDefaultValue);

    theWizard.setField(QString("URL"), varDefault);
    theWizard.setField(QString("contractType"), QString("asset")); // So the wizard knows it's creating an asset contract.

    QString qstrDefaultContract(
                "<instrumentDefinition version=\"2.0\">\n"
                "\n"
                "<entity shortname=\"Fed\"\n"
                " longname=\"The Rothschilds\"\n"
                " email=\"rulers@highplaces.com\"/>\n"
                        "\n"
                "<issue company=\"Federal Reserve, a private company\"\n"
                " email=\"bubbles@fed.gov\"\n"
                " contractUrl=\"https://fed.gov/sucker\"\n"
                " type=\"currency\"/>\n"
                       "\n"
                "<currency name=\"US Dollars\" tla=\"USD\" symbol=\"$\" type=\"decimal\" factor=\"100\" decimalPower=\"2\" fraction=\"cents\"/>\n"
                                "\n"
                "</instrumentDefinition>\n"
    );

    theWizard.setField(QString("contractXML"), qstrDefaultContract);

    if (QDialog::Accepted == theWizard.exec()) // <=== EXECUTING WIZARD HERE
    {
        bool bIsImporting = theWizard.field("isImporting").toBool();
        bool bIsCreating  = theWizard.field("isCreating").toBool();

        if (bIsImporting)
        {
            bool bIsURL      = theWizard.field("isURL").toBool();
            bool bIsFilename = theWizard.field("isFilename").toBool();
            bool bIsContents = theWizard.field("isContents").toBool();

            if (bIsURL)
            {
                QString qstrURL = theWizard.field("URL").toString();
                // --------------------------------
                if (qstrURL.isEmpty())
                {
                    QMessageBox::warning(this, tr("URL is Empty"),
                        tr("No URL was provided."));

                    return;
                }

                QUrl theURL(qstrURL);
                // --------------------------------
                if (m_pDownloader)
                {
                    m_pDownloader->deleteLater();
                    m_pDownloader = NULL;
                }
                // --------------------------------
                m_pDownloader = new FileDownloader(theURL, this);

                connect(m_pDownloader, SIGNAL(downloaded()), SLOT(DownloadedURL()));
            }
            // --------------------------------
            else if (bIsFilename)
            {
                QString fileName = theWizard.field("Filename").toString();

                if (fileName.isEmpty())
                {
                    QMessageBox::warning(this, tr("Filename is Empty"),
                        tr("No filename was provided."));

                    return;
                }
                // -----------------------------------------------
                QString qstrContents;
                QFile plainFile(fileName);

                if (plainFile.open(QIODevice::ReadOnly))//| QIODevice::Text)) // Text flag translates /n/r to /n
                {
                    QTextStream in(&plainFile); // Todo security: check filesize here and place a maximum size.
                    qstrContents = in.readAll();

                    plainFile.close();
                    // ----------------------------
                    if (qstrContents.isEmpty())
                    {
                        QMessageBox::warning(this, tr("File Was Empty"),
                                             QString("%1: %2").arg(tr("File was apparently empty")).arg(fileName));
                        return;
                    }
                    // ----------------------------
                }
                else
                {
                    QMessageBox::warning(this, tr("Failed Reading File"),
                                         QString("%1: %2").arg(tr("Failed trying to read file")).arg(fileName));
                    return;
                }
                // -----------------------------------------------
                ImportContract(qstrContents);
            }
            // --------------------------------
            else if (bIsContents)
            {
                QString qstrContents = theWizard.getContents();

                if (qstrContents.isEmpty())
                {
                    QMessageBox::warning(this, tr("Empty Contract"),
                        tr("Failure Importing: Contract is Empty."));
                    return;
                }
                // -------------------------
                ImportContract(qstrContents);
            }
        }
        // --------------------------------
        else if (bIsCreating)
        {
            //QString qstrXMLContents = theWizard.field("contractXML").toString();
            QString qstrNymID       = theWizard.field("NymID").toString();
            QString qstrTerms       = theWizard.field("terms").toString();

            if (qstrTerms.isEmpty())
            {
                qstrTerms = QString(tr("THIS SPACE INTENTIONALLY LEFT BLANK"));
            }

            const bool bIsCurrency = theWizard.field("asset_type_currency").toBool();
            const bool bIsSecurity = theWizard.field("asset_type_security").toBool();
            const bool bIsBasket   = theWizard.field("asset_type_basket")  .toBool();

            QString qstrContractName;
            QString qstrPrimaryUnit;
            QString qstrSymbol;
            QString qstrTLA;
            QString qstrFractionalUnit;
            QString qstrDecimalSpaces;

            std::string strContractID;

            if (bIsCurrency)
            {
                qstrContractName   = theWizard.field("currency_contract_name").toString();
                qstrPrimaryUnit    = theWizard.field("currency_primary_unit").toString();
                qstrSymbol         = theWizard.field("currency_symbol").toString();
                qstrTLA            = theWizard.field("currency_tla").toString();
                qstrFractionalUnit = theWizard.field("currency_fractional_unit").toString();
                qstrDecimalSpaces  = theWizard.field("currency_decimal_spaces").toString();
                std::string str_decimal_spaces = qstrDecimalSpaces.toStdString();
                opentxs::String strDecimalSpaces(str_decimal_spaces);

//                EXPORT std::string CreateCurrencyContract(
//                      const std::string& NYM_ID,
//                      const std::string& shortname,
//                      const std::string& terms,
//                      const std::string& name,
//                      const std::string& symbol,
//                      const std::string& tla,
//                      const uint32_t power,
//                      const std::string& fraction) const;

                const std::uint32_t uDecimalSpaces = strDecimalSpaces.ToUint();

                strContractID =
                    Moneychanger::It()->OT().Exec().CreateCurrencyContract(
                        qstrNymID.toStdString(),
                        qstrContractName.toStdString(),  //  "Coinbase Dollars" (refers to the contract.) shortname
                        qstrTerms.toStdString(), // terms
                        qstrPrimaryUnit.toStdString(),  //   Primary unit name "dollars" or "yuan".  name
                        qstrSymbol.toStdString(),  //  Symbol.
                        qstrTLA.toStdString(),  //  "USD", etc.
//                        100,  //   100 cents in a dollar.  Factor. (I think factor is gone now.)
                        uDecimalSpaces,  //  A "cent" is 2 decimal places right of a "dollar." Decimal power.
                        qstrFractionalUnit.toStdString()
                        );
            }
            else if (bIsSecurity)
            {
                qstrContractName   = theWizard.field("security_contract_name").toString();
                qstrPrimaryUnit    = theWizard.field("security_primary_unit").toString();
                qstrSymbol         = theWizard.field("security_symbol").toString();
                //qstrTLA          = theWizard.field("security_tla").toString();

                strContractID =
                    Moneychanger::It()->OT().Exec().CreateSecurityContract(
                        qstrNymID.toStdString(),
                        qstrContractName.toStdString(),  //  "Sample Co. Share" (refers to the contract.)
                        qstrTerms.toStdString(),
                        qstrPrimaryUnit.toStdString(),  //   Primary unit name "shares"
                        qstrSymbol.toStdString());  //  Symbol aka "SAMPL"
            }
            else if (bIsBasket)
            {
                qDebug() << "Unimplemented still. (Basket currency creation wizard.)";
                return;
            }
            // ----------------------------------------------------------
            if ("" == strContractID) {
                QMessageBox::warning(this, tr("Failed Creating Contract"),
                                     tr("Unable to create contract. Perhaps the XML contents were bad?"));
                return;
            }
            else {
                std::string strNewContract = Moneychanger::It()->OT().Exec().GetAssetType_Contract(strContractID);

                if ("" == strNewContract) {
                    QMessageBox::warning(this, tr("Unable to Load"),
                                         tr("While the contract was apparently created, Moneychanger is unable to load it up. (Strange.)"));
                    return;
                }
                else { // Success.
                    QString qstrContractID   = QString::fromStdString(strContractID);
                    QString qstrContractName = QString::fromStdString(Moneychanger::It()->OT().Exec().GetAssetType_Name(strContractID));

                    std::cout << "New asset contract name: " << qstrContractName.toStdString() << std::endl;

                    m_pOwner->m_map.insert(qstrContractID,
                                           qstrContractName);
                    m_pOwner->SetPreSelected(qstrContractID);
                    // ------------------------------------------------
                    emit newAssetAdded(qstrContractID);
                    return;
                }
            }
        }
    }
}

// ------------------------------------------------------

//virtual
void MTAssetDetails::refresh(QString strID, QString strName)
{
    if (!strID.isEmpty() && (NULL != ui))
    {
        QWidget * pHeaderWidget  = MTEditDetails::CreateDetailHeaderWidget(m_Type, strID, strName, "", "", ":/icons/icons/assets.png", false);

        pHeaderWidget->setObjectName(QString("DetailHeader")); // So the stylesheet doesn't get applied to all its sub-widgets.

        if (m_pHeaderWidget)
        {
            ui->verticalLayout->removeWidget(m_pHeaderWidget);

            m_pHeaderWidget->setParent(NULL);
            m_pHeaderWidget->disconnect();
            m_pHeaderWidget->deleteLater();

            m_pHeaderWidget = NULL;
        }
        ui->verticalLayout->insertWidget(0, pHeaderWidget);
        m_pHeaderWidget = pHeaderWidget;
        // ----------------------------------
        QString qstrContents =
            QString::fromStdString(Moneychanger::It()->OT().Exec().
                GetAssetType_Contract(strID.toStdString()));
        opentxs::proto::UnitDefinition contractProto =
            opentxs::proto::StringToProto<opentxs::proto::UnitDefinition>
                (opentxs::String(qstrContents.toStdString()));

        if (m_pPlainTextEdit)
            m_pPlainTextEdit->setPlainText(qstrContents);
        // ----------------------------------
        QString qstrDetails("");

        qstrDetails += contractProto.has_name() ? QString("- %1: %2\n").arg(tr("Name")).arg(QString::fromStdString(contractProto.name())) : QString("");
        qstrDetails += contractProto.has_version() ? QString("- %1: %2\n").arg(tr("Version")).arg(QString::number(contractProto.version())) : QString("");
        qstrDetails += contractProto.has_shortname() ? QString("- %1: %2\n").arg(tr("Short Name")).arg(QString::fromStdString(contractProto.shortname())) : QString("");
        qstrDetails += contractProto.has_symbol() ? QString("- %1: %2\n").arg(tr("Symbol")).arg(QString::fromStdString(contractProto.symbol())) : QString("");

        qstrDetails += QString("\n");

        if (contractProto.has_currency()) {
            qstrDetails += QString("%1:\n").arg(tr("Is currency"));
            auto & params = contractProto.currency();
            qstrDetails += params.has_tla() ? QString("- %1: %2\n").arg(tr("TLA")).arg(QString::fromStdString(params.tla())) : QString("");
            qstrDetails += params.has_version() ? QString("- %1: %2\n").arg(tr("Version")).arg(QString::number(params.version())) : QString("");
            qstrDetails += params.has_fraction() ? QString("- %1: %2\n").arg(tr("Fraction")).arg(QString::fromStdString(params.fraction())) : QString("");
            qstrDetails += params.has_power() ? QString("- %1: %2\n").arg(tr("Power")).arg(QString::number(params.power())) : QString("");
        }
        if (contractProto.has_security()) {
            qstrDetails += QString("%1:\n").arg(tr("Is shares"));
            auto & params = contractProto.security();
            qstrDetails += params.has_version() ? QString("- %1: %2\n").arg(tr("Version")).arg(QString::number(params.version())) : QString("");
        }
        if (contractProto.has_basket()) {
            qstrDetails += QString("%1:\n").arg(tr("Is basket"));
            auto & params = contractProto.basket();
            qstrDetails += params.has_version() ? QString("- %1: %2\n").arg(tr("Version")).arg(QString::number(params.version())) : QString("");
        }

        qstrDetails += contractProto.has_terms() ? QString("\n- %1:\n%2").arg(tr("Terms")).arg(QString::fromStdString(contractProto.terms())) : QString("");

        ui->plainTextEditDetails->setPlainText(qstrDetails);
        // ----------------------------------
        QString qstrNymID("");

        ui->pushButton->setVisible(false);

        if (!qstrContents.isEmpty()) {
            std::string str_signer_nym = contractProto.nymid();

            if (!str_signer_nym.empty()) {
                qstrNymID = QString::fromStdString(str_signer_nym);
                // --------------------------
                if (Moneychanger::It()->OT().Exec().VerifyUserPrivateKey(str_signer_nym)) {
                    ui->pushButton->setVisible(true);
                }
            }
        }
        // --------------------------
        ui->lineEditID   ->setText(strID);
        ui->lineEditName ->setText(strName);
        ui->lineEditNymID->setText(qstrNymID);

        FavorLeftSideForIDs();
    }
}

// ------------------------------------------------------

void MTAssetDetails::on_lineEditName_editingFinished()
{
    if (!m_pOwner->m_qstrCurrentID.isEmpty())
    {
        bool bSuccess = Moneychanger::It()->OT().Exec().SetAssetType_Name(m_pOwner->m_qstrCurrentID.toStdString(),  // Asset Type
                                                      ui->lineEditName->text(). toStdString()); // New Name
        if (bSuccess)
        {
            m_pOwner->m_map.remove(m_pOwner->m_qstrCurrentID);
            m_pOwner->m_map.insert(m_pOwner->m_qstrCurrentID, ui->lineEditName->text());

            m_pOwner->SetPreSelected(m_pOwner->m_qstrCurrentID);
            // ------------------------------------------------
            emit assetsChanged();
            // ------------------------------------------------
        }
    }
}

// ------------------------------------------------------

