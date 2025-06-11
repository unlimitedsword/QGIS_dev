/********************************************************************************
** Form generated from reading UI file 'qgsgroupwmsdatadialogbase.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QGSGROUPWMSDATADIALOGBASE_H
#define UI_QGSGROUPWMSDATADIALOGBASE_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "qgsmaplayerserverpropertieswidget.h"

QT_BEGIN_NAMESPACE

class Ui_QgsGroupWMSDataDialogBase
{
public:
    QVBoxLayout *verticalLayout;
    QgsMapLayerServerPropertiesWidget *mMapLayerServerPropertiesWidget;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *QgsGroupWMSDataDialogBase)
    {
        if (QgsGroupWMSDataDialogBase->objectName().isEmpty())
            QgsGroupWMSDataDialogBase->setObjectName(QString::fromUtf8("QgsGroupWMSDataDialogBase"));
        QgsGroupWMSDataDialogBase->setWindowModality(Qt::WindowModal);
        QIcon icon;
        icon.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        QgsGroupWMSDataDialogBase->setWindowIcon(icon);
        verticalLayout = new QVBoxLayout(QgsGroupWMSDataDialogBase);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        mMapLayerServerPropertiesWidget = new QgsMapLayerServerPropertiesWidget(QgsGroupWMSDataDialogBase);
        mMapLayerServerPropertiesWidget->setObjectName(QString::fromUtf8("mMapLayerServerPropertiesWidget"));

        verticalLayout->addWidget(mMapLayerServerPropertiesWidget);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(QgsGroupWMSDataDialogBase);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(QgsGroupWMSDataDialogBase);
        QObject::connect(buttonBox, SIGNAL(accepted()), QgsGroupWMSDataDialogBase, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), QgsGroupWMSDataDialogBase, SLOT(reject()));

        QMetaObject::connectSlotsByName(QgsGroupWMSDataDialogBase);
    } // setupUi

    void retranslateUi(QDialog *QgsGroupWMSDataDialogBase)
    {
        QgsGroupWMSDataDialogBase->setWindowTitle(QCoreApplication::translate("QgsGroupWMSDataDialogBase", "Set Group WMS Data", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QgsGroupWMSDataDialogBase: public Ui_QgsGroupWMSDataDialogBase {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QGSGROUPWMSDATADIALOGBASE_H
