#ifndef WIZARDNEWOFFER_HPP
#define WIZARDNEWOFFER_HPP

#include "core/WinsockWrapper.h"
#include "core/ExportWrapper.h"

#include <QWizard>

namespace Ui {
class WizardNewOffer;
}

class WizardNewOffer : public QWizard
{
    Q_OBJECT

public:
    explicit WizardNewOffer(QWidget *parent = 0);
    ~WizardNewOffer();

    void SetNymID     (QString qstrID)   { m_nymID      = qstrID;   }
    void SetNymName   (QString qstrName) { m_nymName    = qstrName; }
    void SetNotaryID  (QString qstrID)   { m_NotaryID   = qstrID;   }
    void SetServerName(QString qstrName) { m_serverName = qstrName; }

    QString GetNymID()      const { return m_nymID;      }
    QString GetNymName()    const { return m_nymName;    }
    QString GetNotaryID()   const { return m_NotaryID;   }
    QString GetServerName() const { return m_serverName; }

private:
    QString m_nymID;
    QString m_nymName;
    QString m_NotaryID;
    QString m_serverName;

    Ui::WizardNewOffer *ui;
};

#endif // WIZARDNEWOFFER_HPP
