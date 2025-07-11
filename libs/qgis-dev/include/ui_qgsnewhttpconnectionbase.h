/********************************************************************************
** Form generated from reading UI file 'qgsnewhttpconnectionbase.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QGSNEWHTTPCONNECTIONBASE_H
#define UI_QGSNEWHTTPCONNECTIONBASE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include "qgsauthsettingswidget.h"
#include "qgshttpheaderwidget.h"

QT_BEGIN_NAMESPACE

class Ui_QgsNewHttpConnectionBase
{
public:
    QGridLayout *gridLayout_4;
    QGroupBox *mGroupBox;
    QGridLayout *gridLayout;
    QGroupBox *mWfsOptionsGroupBox;
    QGridLayout *gridLayout1;
    QLabel *lblFeatureMode;
    QComboBox *cmbVersion;
    QCheckBox *cbxWfsInvertAxisOrientation;
    QComboBox *mComboHttpMethod;
    QCheckBox *cbxWfsUseGml2EncodingForTransactions;
    QLabel *lblPageSize;
    QLabel *lblMaxNumFeatures;
    QLabel *lblVersion;
    QCheckBox *cbxWfsIgnoreAxisOrientation;
    QLabel *lblVersion_2;
    QLineEdit *txtMaxNumFeatures;
    QPushButton *mWfsVersionDetectButton;
    QLineEdit *txtPageSize;
    QComboBox *cmbFeaturePaging;
    QComboBox *mComboWfsFeatureMode;
    QLabel *lblFeaturePaging;
    QPushButton *mTestConnectionButton;
    QSpacerItem *verticalSpacer;
    QGroupBox *mWmsOptionsGroupBox;
    QGridLayout *gridLayout_2;
    QCheckBox *cbxSmoothPixmapTransform;
    QCheckBox *cbxWmsIgnoreAxisOrientation;
    QComboBox *cmbTilePixelRatio;
    QLabel *lblFeatureCount;
    QCheckBox *cbxWmsInvertAxisOrientation;
    QCheckBox *cbxIgnoreGetFeatureInfoURI;
    QLabel *lblDpiMode;
    QCheckBox *cbxWmsIgnoreReportedLayerExtents;
    QComboBox *cmbDpiMode;
    QLabel *lblTilePixelRatio;
    QCheckBox *cbxIgnoreGetMapURI;
    QSpinBox *sbFeatureCount;
    QgsHttpHeaderWidget *mHttpHeaders;
    QFrame *frame;
    QGridLayout *gridLayout_5;
    QLabel *TextLabel1_2;
    QLineEdit *txtName;
    QLabel *TextLabel1;
    QLineEdit *txtUrl;
    QGroupBox *mAuthGroupBox;
    QVBoxLayout *verticalLayout;
    QgsAuthSettingsWidget *mAuthSettings;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *QgsNewHttpConnectionBase)
    {
        if (QgsNewHttpConnectionBase->objectName().isEmpty())
            QgsNewHttpConnectionBase->setObjectName(QString::fromUtf8("QgsNewHttpConnectionBase"));
        QgsNewHttpConnectionBase->resize(567, 821);
        QgsNewHttpConnectionBase->setSizeGripEnabled(true);
        QgsNewHttpConnectionBase->setModal(true);
        gridLayout_4 = new QGridLayout(QgsNewHttpConnectionBase);
        gridLayout_4->setSpacing(6);
        gridLayout_4->setContentsMargins(11, 11, 11, 11);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        mGroupBox = new QGroupBox(QgsNewHttpConnectionBase);
        mGroupBox->setObjectName(QString::fromUtf8("mGroupBox"));
        gridLayout = new QGridLayout(mGroupBox);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        mWfsOptionsGroupBox = new QGroupBox(mGroupBox);
        mWfsOptionsGroupBox->setObjectName(QString::fromUtf8("mWfsOptionsGroupBox"));
        gridLayout1 = new QGridLayout(mWfsOptionsGroupBox);
        gridLayout1->setSpacing(6);
        gridLayout1->setContentsMargins(11, 11, 11, 11);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        lblFeatureMode = new QLabel(mWfsOptionsGroupBox);
        lblFeatureMode->setObjectName(QString::fromUtf8("lblFeatureMode"));

        gridLayout1->addWidget(lblFeatureMode, 5, 0, 1, 1);

        cmbVersion = new QComboBox(mWfsOptionsGroupBox);
        cmbVersion->setObjectName(QString::fromUtf8("cmbVersion"));

        gridLayout1->addWidget(cmbVersion, 0, 1, 1, 1);

        cbxWfsInvertAxisOrientation = new QCheckBox(mWfsOptionsGroupBox);
        cbxWfsInvertAxisOrientation->setObjectName(QString::fromUtf8("cbxWfsInvertAxisOrientation"));

        gridLayout1->addWidget(cbxWfsInvertAxisOrientation, 7, 0, 1, 3);

        mComboHttpMethod = new QComboBox(mWfsOptionsGroupBox);
        mComboHttpMethod->setObjectName(QString::fromUtf8("mComboHttpMethod"));

        gridLayout1->addWidget(mComboHttpMethod, 1, 1, 1, 2);

        cbxWfsUseGml2EncodingForTransactions = new QCheckBox(mWfsOptionsGroupBox);
        cbxWfsUseGml2EncodingForTransactions->setObjectName(QString::fromUtf8("cbxWfsUseGml2EncodingForTransactions"));

        gridLayout1->addWidget(cbxWfsUseGml2EncodingForTransactions, 8, 0, 1, 2);

        lblPageSize = new QLabel(mWfsOptionsGroupBox);
        lblPageSize->setObjectName(QString::fromUtf8("lblPageSize"));

        gridLayout1->addWidget(lblPageSize, 4, 0, 1, 1);

        lblMaxNumFeatures = new QLabel(mWfsOptionsGroupBox);
        lblMaxNumFeatures->setObjectName(QString::fromUtf8("lblMaxNumFeatures"));

        gridLayout1->addWidget(lblMaxNumFeatures, 2, 0, 1, 1);

        lblVersion = new QLabel(mWfsOptionsGroupBox);
        lblVersion->setObjectName(QString::fromUtf8("lblVersion"));

        gridLayout1->addWidget(lblVersion, 0, 0, 1, 1);

        cbxWfsIgnoreAxisOrientation = new QCheckBox(mWfsOptionsGroupBox);
        cbxWfsIgnoreAxisOrientation->setObjectName(QString::fromUtf8("cbxWfsIgnoreAxisOrientation"));

        gridLayout1->addWidget(cbxWfsIgnoreAxisOrientation, 6, 0, 1, 3);

        lblVersion_2 = new QLabel(mWfsOptionsGroupBox);
        lblVersion_2->setObjectName(QString::fromUtf8("lblVersion_2"));

        gridLayout1->addWidget(lblVersion_2, 1, 0, 1, 1);

        txtMaxNumFeatures = new QLineEdit(mWfsOptionsGroupBox);
        txtMaxNumFeatures->setObjectName(QString::fromUtf8("txtMaxNumFeatures"));

        gridLayout1->addWidget(txtMaxNumFeatures, 2, 1, 1, 2);

        mWfsVersionDetectButton = new QPushButton(mWfsOptionsGroupBox);
        mWfsVersionDetectButton->setObjectName(QString::fromUtf8("mWfsVersionDetectButton"));

        gridLayout1->addWidget(mWfsVersionDetectButton, 0, 2, 1, 1);

        txtPageSize = new QLineEdit(mWfsOptionsGroupBox);
        txtPageSize->setObjectName(QString::fromUtf8("txtPageSize"));

        gridLayout1->addWidget(txtPageSize, 4, 1, 1, 2);

        cmbFeaturePaging = new QComboBox(mWfsOptionsGroupBox);
        cmbFeaturePaging->setObjectName(QString::fromUtf8("cmbFeaturePaging"));

        gridLayout1->addWidget(cmbFeaturePaging, 3, 1, 1, 2);

        mComboWfsFeatureMode = new QComboBox(mWfsOptionsGroupBox);
        mComboWfsFeatureMode->setObjectName(QString::fromUtf8("mComboWfsFeatureMode"));

        gridLayout1->addWidget(mComboWfsFeatureMode, 5, 1, 1, 2);

        lblFeaturePaging = new QLabel(mWfsOptionsGroupBox);
        lblFeaturePaging->setObjectName(QString::fromUtf8("lblFeaturePaging"));

        gridLayout1->addWidget(lblFeaturePaging, 3, 0, 1, 1);


        gridLayout->addWidget(mWfsOptionsGroupBox, 3, 0, 1, 2);

        mTestConnectionButton = new QPushButton(mGroupBox);
        mTestConnectionButton->setObjectName(QString::fromUtf8("mTestConnectionButton"));

        gridLayout->addWidget(mTestConnectionButton, 5, 0, 1, 2);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 6, 0, 1, 2);

        mWmsOptionsGroupBox = new QGroupBox(mGroupBox);
        mWmsOptionsGroupBox->setObjectName(QString::fromUtf8("mWmsOptionsGroupBox"));
        gridLayout_2 = new QGridLayout(mWmsOptionsGroupBox);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        cbxSmoothPixmapTransform = new QCheckBox(mWmsOptionsGroupBox);
        cbxSmoothPixmapTransform->setObjectName(QString::fromUtf8("cbxSmoothPixmapTransform"));

        gridLayout_2->addWidget(cbxSmoothPixmapTransform, 13, 0, 1, 2);

        cbxWmsIgnoreAxisOrientation = new QCheckBox(mWmsOptionsGroupBox);
        cbxWmsIgnoreAxisOrientation->setObjectName(QString::fromUtf8("cbxWmsIgnoreAxisOrientation"));

        gridLayout_2->addWidget(cbxWmsIgnoreAxisOrientation, 8, 0, 1, 2);

        cmbTilePixelRatio = new QComboBox(mWmsOptionsGroupBox);
        cmbTilePixelRatio->setObjectName(QString::fromUtf8("cmbTilePixelRatio"));

        gridLayout_2->addWidget(cmbTilePixelRatio, 1, 1, 1, 1);

        lblFeatureCount = new QLabel(mWmsOptionsGroupBox);
        lblFeatureCount->setObjectName(QString::fromUtf8("lblFeatureCount"));

        gridLayout_2->addWidget(lblFeatureCount, 2, 0, 1, 1);

        cbxWmsInvertAxisOrientation = new QCheckBox(mWmsOptionsGroupBox);
        cbxWmsInvertAxisOrientation->setObjectName(QString::fromUtf8("cbxWmsInvertAxisOrientation"));

        gridLayout_2->addWidget(cbxWmsInvertAxisOrientation, 9, 0, 1, 2);

        cbxIgnoreGetFeatureInfoURI = new QCheckBox(mWmsOptionsGroupBox);
        cbxIgnoreGetFeatureInfoURI->setObjectName(QString::fromUtf8("cbxIgnoreGetFeatureInfoURI"));

        gridLayout_2->addWidget(cbxIgnoreGetFeatureInfoURI, 6, 0, 1, 2);

        lblDpiMode = new QLabel(mWmsOptionsGroupBox);
        lblDpiMode->setObjectName(QString::fromUtf8("lblDpiMode"));

        gridLayout_2->addWidget(lblDpiMode, 0, 0, 1, 1);

        cbxWmsIgnoreReportedLayerExtents = new QCheckBox(mWmsOptionsGroupBox);
        cbxWmsIgnoreReportedLayerExtents->setObjectName(QString::fromUtf8("cbxWmsIgnoreReportedLayerExtents"));

        gridLayout_2->addWidget(cbxWmsIgnoreReportedLayerExtents, 7, 0, 1, 2);

        cmbDpiMode = new QComboBox(mWmsOptionsGroupBox);
        cmbDpiMode->setObjectName(QString::fromUtf8("cmbDpiMode"));

        gridLayout_2->addWidget(cmbDpiMode, 0, 1, 1, 1);

        lblTilePixelRatio = new QLabel(mWmsOptionsGroupBox);
        lblTilePixelRatio->setObjectName(QString::fromUtf8("lblTilePixelRatio"));

        gridLayout_2->addWidget(lblTilePixelRatio, 1, 0, 1, 1);

        cbxIgnoreGetMapURI = new QCheckBox(mWmsOptionsGroupBox);
        cbxIgnoreGetMapURI->setObjectName(QString::fromUtf8("cbxIgnoreGetMapURI"));

        gridLayout_2->addWidget(cbxIgnoreGetMapURI, 5, 0, 1, 2);

        sbFeatureCount = new QSpinBox(mWmsOptionsGroupBox);
        sbFeatureCount->setObjectName(QString::fromUtf8("sbFeatureCount"));
        sbFeatureCount->setMinimum(0);
        sbFeatureCount->setMaximum(1000);
        sbFeatureCount->setValue(10);

        gridLayout_2->addWidget(sbFeatureCount, 2, 1, 1, 1);


        gridLayout->addWidget(mWmsOptionsGroupBox, 4, 0, 1, 2);

        mHttpHeaders = new QgsHttpHeaderWidget(mGroupBox);
        mHttpHeaders->setObjectName(QString::fromUtf8("mHttpHeaders"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(mHttpHeaders->sizePolicy().hasHeightForWidth());
        mHttpHeaders->setSizePolicy(sizePolicy);

        gridLayout->addWidget(mHttpHeaders, 2, 0, 1, 1);

        frame = new QFrame(mGroupBox);
        frame->setObjectName(QString::fromUtf8("frame"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(frame->sizePolicy().hasHeightForWidth());
        frame->setSizePolicy(sizePolicy1);
        gridLayout_5 = new QGridLayout(frame);
        gridLayout_5->setSpacing(6);
        gridLayout_5->setContentsMargins(11, 11, 11, 11);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        gridLayout_5->setContentsMargins(0, 0, 0, 0);
        TextLabel1_2 = new QLabel(frame);
        TextLabel1_2->setObjectName(QString::fromUtf8("TextLabel1_2"));
        TextLabel1_2->setWordWrap(true);
        TextLabel1_2->setMargin(5);

        gridLayout_5->addWidget(TextLabel1_2, 0, 0, 1, 1);

        txtName = new QLineEdit(frame);
        txtName->setObjectName(QString::fromUtf8("txtName"));
        txtName->setMinimumSize(QSize(0, 0));
        txtName->setFrame(true);

        gridLayout_5->addWidget(txtName, 0, 1, 1, 1);

        TextLabel1 = new QLabel(frame);
        TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));
        TextLabel1->setMargin(5);

        gridLayout_5->addWidget(TextLabel1, 1, 0, 1, 1);

        txtUrl = new QLineEdit(frame);
        txtUrl->setObjectName(QString::fromUtf8("txtUrl"));

        gridLayout_5->addWidget(txtUrl, 1, 1, 1, 1);


        gridLayout->addWidget(frame, 0, 0, 1, 2);

        mAuthGroupBox = new QGroupBox(mGroupBox);
        mAuthGroupBox->setObjectName(QString::fromUtf8("mAuthGroupBox"));
        verticalLayout = new QVBoxLayout(mAuthGroupBox);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(6, 6, 6, 6);
        mAuthSettings = new QgsAuthSettingsWidget(mAuthGroupBox);
        mAuthSettings->setObjectName(QString::fromUtf8("mAuthSettings"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(mAuthSettings->sizePolicy().hasHeightForWidth());
        mAuthSettings->setSizePolicy(sizePolicy2);

        verticalLayout->addWidget(mAuthSettings);


        gridLayout->addWidget(mAuthGroupBox, 1, 0, 1, 2);


        gridLayout_4->addWidget(mGroupBox, 0, 0, 1, 1);

        buttonBox = new QDialogButtonBox(QgsNewHttpConnectionBase);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Help|QDialogButtonBox::Ok);

        gridLayout_4->addWidget(buttonBox, 1, 0, 1, 1);

#if QT_CONFIG(shortcut)
        lblDpiMode->setBuddy(cmbDpiMode);
        lblTilePixelRatio->setBuddy(cmbTilePixelRatio);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(txtName, txtUrl);
        QWidget::setTabOrder(txtUrl, cmbVersion);
        QWidget::setTabOrder(cmbVersion, mWfsVersionDetectButton);
        QWidget::setTabOrder(mWfsVersionDetectButton, mComboHttpMethod);
        QWidget::setTabOrder(mComboHttpMethod, txtMaxNumFeatures);
        QWidget::setTabOrder(txtMaxNumFeatures, cmbFeaturePaging);
        QWidget::setTabOrder(cmbFeaturePaging, txtPageSize);
        QWidget::setTabOrder(txtPageSize, mComboWfsFeatureMode);
        QWidget::setTabOrder(mComboWfsFeatureMode, cbxWfsIgnoreAxisOrientation);
        QWidget::setTabOrder(cbxWfsIgnoreAxisOrientation, cbxWfsInvertAxisOrientation);
        QWidget::setTabOrder(cbxWfsInvertAxisOrientation, cbxWfsUseGml2EncodingForTransactions);
        QWidget::setTabOrder(cbxWfsUseGml2EncodingForTransactions, cmbDpiMode);
        QWidget::setTabOrder(cmbDpiMode, cmbTilePixelRatio);
        QWidget::setTabOrder(cmbTilePixelRatio, sbFeatureCount);
        QWidget::setTabOrder(sbFeatureCount, cbxIgnoreGetMapURI);
        QWidget::setTabOrder(cbxIgnoreGetMapURI, cbxIgnoreGetFeatureInfoURI);
        QWidget::setTabOrder(cbxIgnoreGetFeatureInfoURI, cbxWmsIgnoreReportedLayerExtents);
        QWidget::setTabOrder(cbxWmsIgnoreReportedLayerExtents, cbxWmsIgnoreAxisOrientation);
        QWidget::setTabOrder(cbxWmsIgnoreAxisOrientation, cbxWmsInvertAxisOrientation);
        QWidget::setTabOrder(cbxWmsInvertAxisOrientation, cbxSmoothPixmapTransform);
        QWidget::setTabOrder(cbxSmoothPixmapTransform, mTestConnectionButton);

        retranslateUi(QgsNewHttpConnectionBase);
        QObject::connect(buttonBox, SIGNAL(accepted()), QgsNewHttpConnectionBase, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), QgsNewHttpConnectionBase, SLOT(reject()));

        QMetaObject::connectSlotsByName(QgsNewHttpConnectionBase);
    } // setupUi

    void retranslateUi(QDialog *QgsNewHttpConnectionBase)
    {
        QgsNewHttpConnectionBase->setWindowTitle(QCoreApplication::translate("QgsNewHttpConnectionBase", "Create a New Connection", nullptr));
        mGroupBox->setTitle(QCoreApplication::translate("QgsNewHttpConnectionBase", "Connection Details", nullptr));
        mWfsOptionsGroupBox->setTitle(QCoreApplication::translate("QgsNewHttpConnectionBase", "WFS Options", nullptr));
        lblFeatureMode->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "<html><head/><body><p>Feature mode<br/>(Simple vs Complex)</p></body></html>", nullptr));
        cbxWfsInvertAxisOrientation->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Invert axis orientation", nullptr));
#if QT_CONFIG(tooltip)
        cbxWfsUseGml2EncodingForTransactions->setToolTip(QCoreApplication::translate("QgsNewHttpConnectionBase", "<html><head/><body><p>This might be necessary on some <span style=\" font-weight:600;\">broken</span> ESRI map servers when using WFS-T 1.1.0.</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        cbxWfsUseGml2EncodingForTransactions->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Use GML2 encoding for transactions", nullptr));
        lblPageSize->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Page size", nullptr));
        lblMaxNumFeatures->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Max. number of features", nullptr));
        lblVersion->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Version", nullptr));
        cbxWfsIgnoreAxisOrientation->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Ignore axis orientation (WFS 1.1/WFS 2.0)", nullptr));
        lblVersion_2->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Preferred HTTP method", nullptr));
#if QT_CONFIG(tooltip)
        txtMaxNumFeatures->setToolTip(QCoreApplication::translate("QgsNewHttpConnectionBase", "<html><head/><body><p>Enter a number to limit the maximum number of features retrieved per feature request. If let to empty, no limit is set.</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        mWfsVersionDetectButton->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Detect", nullptr));
#if QT_CONFIG(tooltip)
        txtPageSize->setToolTip(QCoreApplication::translate("QgsNewHttpConnectionBase", "<html><head/><body><p>Enter a number to limit the maximum number of features retrieved in a single GetFeature request when paging is enabled. If let to empty, server default will apply.</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        lblFeaturePaging->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Feature paging", nullptr));
        mTestConnectionButton->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "&Test Connection", nullptr));
        mWmsOptionsGroupBox->setTitle(QCoreApplication::translate("QgsNewHttpConnectionBase", "WMS/WMTS Options", nullptr));
        cbxSmoothPixmapTransform->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Smooth pixmap transform", nullptr));
        cbxWmsIgnoreAxisOrientation->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Ignore axis orientation (WMS 1.3/WMTS)", nullptr));
        lblFeatureCount->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Maximum number of GetFeatureInfo results", nullptr));
        cbxWmsInvertAxisOrientation->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Invert axis orientation", nullptr));
        cbxIgnoreGetFeatureInfoURI->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Ignore GetFeatureInfo URI reported in capabilities", nullptr));
        lblDpiMode->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "WMS DPI-&Mode", nullptr));
        cbxWmsIgnoreReportedLayerExtents->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Ignore reported layer extents", nullptr));
        lblTilePixelRatio->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "WMTS server-side tile pixel ratio", nullptr));
        cbxIgnoreGetMapURI->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Ignore GetMap/GetTile/GetLegendGraphic URI reported in capabilities", nullptr));
#if QT_CONFIG(tooltip)
        sbFeatureCount->setToolTip(QCoreApplication::translate("QgsNewHttpConnectionBase", "<html><head/><body><p>Specify a default value for FEATURE_COUNT when a new layer is created from this connection. </p><p>FEATURE_COUNT defines the maximum number of results returned by a GetFeatureInfo request, if not specified the server default value (usually 1) will be used.</p><p>Set to 0 to use server default: no FEATURE_COUNT parameter will be added to the request.</p><p><br/></p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        sbFeatureCount->setSpecialValueText(QCoreApplication::translate("QgsNewHttpConnectionBase", "server default", nullptr));
        TextLabel1_2->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "Name", nullptr));
#if QT_CONFIG(tooltip)
        txtName->setToolTip(QCoreApplication::translate("QgsNewHttpConnectionBase", "Name of the new connection", nullptr));
#endif // QT_CONFIG(tooltip)
        TextLabel1->setText(QCoreApplication::translate("QgsNewHttpConnectionBase", "URL", nullptr));
#if QT_CONFIG(tooltip)
        txtUrl->setToolTip(QCoreApplication::translate("QgsNewHttpConnectionBase", "HTTP address of the Web Map Server", nullptr));
#endif // QT_CONFIG(tooltip)
        mAuthGroupBox->setTitle(QCoreApplication::translate("QgsNewHttpConnectionBase", "Authentication", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QgsNewHttpConnectionBase: public Ui_QgsNewHttpConnectionBase {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QGSNEWHTTPCONNECTIONBASE_H
