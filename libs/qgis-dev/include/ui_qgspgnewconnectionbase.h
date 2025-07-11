/********************************************************************************
** Form generated from reading UI file 'qgspgnewconnectionbase.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QGSPGNEWCONNECTIONBASE_H
#define UI_QGSPGNEWCONNECTIONBASE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "qgsauthsettingswidget.h"
#include "qgsfilterlineedit.h"
#include "qgsmessagebar.h"

QT_BEGIN_NAMESPACE

class Ui_QgsPgNewConnectionBase
{
public:
    QGridLayout *gridLayout_3;
    QGroupBox *GroupBox1;
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout_2;
    QLineEdit *txtService;
    QLineEdit *txtPort;
    QLabel *TextLabel3_3;
    QLineEdit *txtName;
    QLineEdit *txtHost;
    QLineEdit *txtDatabase;
    QLabel *TextLabel2;
    QLabel *TextLabel1_2;
    QLabel *TextLabel1;
    QComboBox *cbxSSLmode;
    QLabel *label;
    QLabel *TextLabel2_2;
    QLineEdit *txtSessionRole;
    QLabel *label_2;
    QGroupBox *mAuthGroupBox;
    QGridLayout *gridLayout;
    QgsAuthSettingsWidget *mAuthSettings;
    QPushButton *btnConnect;
    QSpacerItem *verticalSpacer;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_4;
    QCheckBox *cb_allowGeometrylessTables;
    QCheckBox *cb_projectsInDatabase;
    QCheckBox *cb_geometryColumnsOnly;
    QCheckBox *cb_dontResolveType;
    QSpacerItem *verticalSpacer_2;
    QCheckBox *cb_metadataInDatabase;
    QLabel *TextLabel3_5;
    QgsFilterLineEdit *txtSchema;
    QCheckBox *cb_allowRasterOverviewTables;
    QCheckBox *cb_useEstimatedMetadata;
    QCheckBox *cb_publicSchemaOnly;
    QDialogButtonBox *buttonBox;
    QgsMessageBar *bar;

    void setupUi(QDialog *QgsPgNewConnectionBase)
    {
        if (QgsPgNewConnectionBase->objectName().isEmpty())
            QgsPgNewConnectionBase->setObjectName(QString::fromUtf8("QgsPgNewConnectionBase"));
        QgsPgNewConnectionBase->resize(821, 664);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QgsPgNewConnectionBase->sizePolicy().hasHeightForWidth());
        QgsPgNewConnectionBase->setSizePolicy(sizePolicy);
        QgsPgNewConnectionBase->setSizeGripEnabled(true);
        QgsPgNewConnectionBase->setModal(true);
        gridLayout_3 = new QGridLayout(QgsPgNewConnectionBase);
        gridLayout_3->setSpacing(6);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        gridLayout_3->setContentsMargins(-1, 0, -1, -1);
        GroupBox1 = new QGroupBox(QgsPgNewConnectionBase);
        GroupBox1->setObjectName(QString::fromUtf8("GroupBox1"));
        verticalLayout = new QVBoxLayout(GroupBox1);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        gridLayout_2 = new QGridLayout();
        gridLayout_2->setSpacing(6);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        txtService = new QLineEdit(GroupBox1);
        txtService->setObjectName(QString::fromUtf8("txtService"));

        gridLayout_2->addWidget(txtService, 1, 1, 1, 1);

        txtPort = new QLineEdit(GroupBox1);
        txtPort->setObjectName(QString::fromUtf8("txtPort"));

        gridLayout_2->addWidget(txtPort, 3, 1, 1, 1);

        TextLabel3_3 = new QLabel(GroupBox1);
        TextLabel3_3->setObjectName(QString::fromUtf8("TextLabel3_3"));

        gridLayout_2->addWidget(TextLabel3_3, 5, 0, 1, 1);

        txtName = new QLineEdit(GroupBox1);
        txtName->setObjectName(QString::fromUtf8("txtName"));

        gridLayout_2->addWidget(txtName, 0, 1, 1, 1);

        txtHost = new QLineEdit(GroupBox1);
        txtHost->setObjectName(QString::fromUtf8("txtHost"));

        gridLayout_2->addWidget(txtHost, 2, 1, 1, 1);

        txtDatabase = new QLineEdit(GroupBox1);
        txtDatabase->setObjectName(QString::fromUtf8("txtDatabase"));

        gridLayout_2->addWidget(txtDatabase, 4, 1, 1, 1);

        TextLabel2 = new QLabel(GroupBox1);
        TextLabel2->setObjectName(QString::fromUtf8("TextLabel2"));

        gridLayout_2->addWidget(TextLabel2, 4, 0, 1, 1);

        TextLabel1_2 = new QLabel(GroupBox1);
        TextLabel1_2->setObjectName(QString::fromUtf8("TextLabel1_2"));

        gridLayout_2->addWidget(TextLabel1_2, 0, 0, 1, 1);

        TextLabel1 = new QLabel(GroupBox1);
        TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));

        gridLayout_2->addWidget(TextLabel1, 2, 0, 1, 1);

        cbxSSLmode = new QComboBox(GroupBox1);
        cbxSSLmode->setObjectName(QString::fromUtf8("cbxSSLmode"));

        gridLayout_2->addWidget(cbxSSLmode, 5, 1, 1, 1);

        label = new QLabel(GroupBox1);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout_2->addWidget(label, 1, 0, 1, 1);

        TextLabel2_2 = new QLabel(GroupBox1);
        TextLabel2_2->setObjectName(QString::fromUtf8("TextLabel2_2"));

        gridLayout_2->addWidget(TextLabel2_2, 3, 0, 1, 1);

        txtSessionRole = new QLineEdit(GroupBox1);
        txtSessionRole->setObjectName(QString::fromUtf8("txtSessionRole"));

        gridLayout_2->addWidget(txtSessionRole, 6, 1, 1, 1);

        label_2 = new QLabel(GroupBox1);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout_2->addWidget(label_2, 6, 0, 1, 1);


        verticalLayout->addLayout(gridLayout_2);

        mAuthGroupBox = new QGroupBox(GroupBox1);
        mAuthGroupBox->setObjectName(QString::fromUtf8("mAuthGroupBox"));
        gridLayout = new QGridLayout(mAuthGroupBox);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(6, 6, 6, 6);
        mAuthSettings = new QgsAuthSettingsWidget(mAuthGroupBox);
        mAuthSettings->setObjectName(QString::fromUtf8("mAuthSettings"));
        mAuthSettings->setFocusPolicy(Qt::StrongFocus);

        gridLayout->addWidget(mAuthSettings, 0, 0, 1, 1);


        verticalLayout->addWidget(mAuthGroupBox);

        btnConnect = new QPushButton(GroupBox1);
        btnConnect->setObjectName(QString::fromUtf8("btnConnect"));

        verticalLayout->addWidget(btnConnect);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        gridLayout_3->addWidget(GroupBox1, 1, 0, 1, 1);

        groupBox = new QGroupBox(QgsPgNewConnectionBase);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_4 = new QGridLayout(groupBox);
        gridLayout_4->setSpacing(6);
        gridLayout_4->setContentsMargins(11, 11, 11, 11);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        cb_allowGeometrylessTables = new QCheckBox(groupBox);
        cb_allowGeometrylessTables->setObjectName(QString::fromUtf8("cb_allowGeometrylessTables"));
        cb_allowGeometrylessTables->setChecked(false);

        gridLayout_4->addWidget(cb_allowGeometrylessTables, 3, 0, 1, 2);

        cb_projectsInDatabase = new QCheckBox(groupBox);
        cb_projectsInDatabase->setObjectName(QString::fromUtf8("cb_projectsInDatabase"));

        gridLayout_4->addWidget(cb_projectsInDatabase, 5, 0, 1, 2);

        cb_geometryColumnsOnly = new QCheckBox(groupBox);
        cb_geometryColumnsOnly->setObjectName(QString::fromUtf8("cb_geometryColumnsOnly"));

        gridLayout_4->addWidget(cb_geometryColumnsOnly, 0, 0, 1, 2);

        cb_dontResolveType = new QCheckBox(groupBox);
        cb_dontResolveType->setObjectName(QString::fromUtf8("cb_dontResolveType"));

        gridLayout_4->addWidget(cb_dontResolveType, 1, 0, 1, 2);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_4->addItem(verticalSpacer_2, 10, 1, 1, 1);

        cb_metadataInDatabase = new QCheckBox(groupBox);
        cb_metadataInDatabase->setObjectName(QString::fromUtf8("cb_metadataInDatabase"));

        gridLayout_4->addWidget(cb_metadataInDatabase, 6, 0, 1, 2);

        TextLabel3_5 = new QLabel(groupBox);
        TextLabel3_5->setObjectName(QString::fromUtf8("TextLabel3_5"));

        gridLayout_4->addWidget(TextLabel3_5, 9, 0, 1, 1);

        txtSchema = new QgsFilterLineEdit(groupBox);
        txtSchema->setObjectName(QString::fromUtf8("txtSchema"));
        txtSchema->setEchoMode(QLineEdit::Normal);

        gridLayout_4->addWidget(txtSchema, 9, 1, 1, 1);

        cb_allowRasterOverviewTables = new QCheckBox(groupBox);
        cb_allowRasterOverviewTables->setObjectName(QString::fromUtf8("cb_allowRasterOverviewTables"));

        gridLayout_4->addWidget(cb_allowRasterOverviewTables, 7, 0, 1, 2);

        cb_useEstimatedMetadata = new QCheckBox(groupBox);
        cb_useEstimatedMetadata->setObjectName(QString::fromUtf8("cb_useEstimatedMetadata"));

        gridLayout_4->addWidget(cb_useEstimatedMetadata, 4, 0, 1, 2);

        cb_publicSchemaOnly = new QCheckBox(groupBox);
        cb_publicSchemaOnly->setObjectName(QString::fromUtf8("cb_publicSchemaOnly"));

        gridLayout_4->addWidget(cb_publicSchemaOnly, 8, 0, 1, 2);


        gridLayout_3->addWidget(groupBox, 1, 1, 1, 1);

        buttonBox = new QDialogButtonBox(QgsPgNewConnectionBase);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Help|QDialogButtonBox::Ok);

        gridLayout_3->addWidget(buttonBox, 2, 0, 1, 2);

        bar = new QgsMessageBar(QgsPgNewConnectionBase);
        bar->setObjectName(QString::fromUtf8("bar"));

        gridLayout_3->addWidget(bar, 0, 0, 1, 2);

        gridLayout_3->setColumnStretch(0, 1);
        gridLayout_3->setColumnStretch(1, 1);
#if QT_CONFIG(shortcut)
        TextLabel3_3->setBuddy(cbxSSLmode);
        TextLabel2->setBuddy(txtDatabase);
        TextLabel1_2->setBuddy(txtName);
        TextLabel1->setBuddy(txtHost);
        label->setBuddy(txtService);
        TextLabel2_2->setBuddy(txtPort);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(txtName, txtService);
        QWidget::setTabOrder(txtService, txtHost);
        QWidget::setTabOrder(txtHost, txtPort);
        QWidget::setTabOrder(txtPort, txtDatabase);
        QWidget::setTabOrder(txtDatabase, cbxSSLmode);
        QWidget::setTabOrder(cbxSSLmode, txtSessionRole);
        QWidget::setTabOrder(txtSessionRole, mAuthSettings);
        QWidget::setTabOrder(mAuthSettings, btnConnect);
        QWidget::setTabOrder(btnConnect, cb_geometryColumnsOnly);
        QWidget::setTabOrder(cb_geometryColumnsOnly, cb_dontResolveType);
        QWidget::setTabOrder(cb_dontResolveType, cb_allowGeometrylessTables);
        QWidget::setTabOrder(cb_allowGeometrylessTables, cb_useEstimatedMetadata);
        QWidget::setTabOrder(cb_useEstimatedMetadata, cb_projectsInDatabase);
        QWidget::setTabOrder(cb_projectsInDatabase, cb_metadataInDatabase);
        QWidget::setTabOrder(cb_metadataInDatabase, cb_allowRasterOverviewTables);
        QWidget::setTabOrder(cb_allowRasterOverviewTables, cb_publicSchemaOnly);
        QWidget::setTabOrder(cb_publicSchemaOnly, txtSchema);

        retranslateUi(QgsPgNewConnectionBase);
        QObject::connect(buttonBox, SIGNAL(rejected()), QgsPgNewConnectionBase, SLOT(reject()));
        QObject::connect(buttonBox, SIGNAL(accepted()), QgsPgNewConnectionBase, SLOT(accept()));

        QMetaObject::connectSlotsByName(QgsPgNewConnectionBase);
    } // setupUi

    void retranslateUi(QDialog *QgsPgNewConnectionBase)
    {
        QgsPgNewConnectionBase->setWindowTitle(QCoreApplication::translate("QgsPgNewConnectionBase", "Create a New PostGIS Connection", nullptr));
        GroupBox1->setTitle(QCoreApplication::translate("QgsPgNewConnectionBase", "Connection Details", nullptr));
        txtPort->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "5432", nullptr));
        TextLabel3_3->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "SSL &mode", nullptr));
#if QT_CONFIG(tooltip)
        txtName->setToolTip(QCoreApplication::translate("QgsPgNewConnectionBase", "Name of the new connection", nullptr));
#endif // QT_CONFIG(tooltip)
        TextLabel2->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "&Database", nullptr));
        TextLabel1_2->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "&Name", nullptr));
        TextLabel1->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Hos&t", nullptr));
        label->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Service", nullptr));
        TextLabel2_2->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Port", nullptr));
        label_2->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Session ROLE", nullptr));
        mAuthGroupBox->setTitle(QCoreApplication::translate("QgsPgNewConnectionBase", "Authentication", nullptr));
        btnConnect->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "&Test Connection", nullptr));
        groupBox->setTitle(QCoreApplication::translate("QgsPgNewConnectionBase", "Database Details", nullptr));
        cb_allowGeometrylessTables->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Also list tables with no geometry", nullptr));
        cb_projectsInDatabase->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Allow saving/loading QGIS projects in the database", nullptr));
#if QT_CONFIG(tooltip)
        cb_geometryColumnsOnly->setToolTip(QCoreApplication::translate("QgsPgNewConnectionBase", "Restricts the displayed tables to those that are found in the layer registries (geometry_columns, geography_columns, topology.layer). This can speed up the initial display of spatial tables.", nullptr));
#endif // QT_CONFIG(tooltip)
        cb_geometryColumnsOnly->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Only show layers in the layer registries", nullptr));
        cb_dontResolveType->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Don't resolve type of unrestricted columns (GEOMETRY)", nullptr));
        cb_metadataInDatabase->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Allow saving/loading QGIS layer metadata in the database", nullptr));
        TextLabel3_5->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Schema", nullptr));
#if QT_CONFIG(tooltip)
        txtSchema->setToolTip(QCoreApplication::translate("QgsPgNewConnectionBase", "If specified, only tables from the matching schema will be fetched and listed for the provider", nullptr));
#endif // QT_CONFIG(tooltip)
        txtSchema->setPlaceholderText(QCoreApplication::translate("QgsPgNewConnectionBase", "Limit to tables from specific schema", nullptr));
        cb_allowRasterOverviewTables->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Also list raster overview tables", nullptr));
#if QT_CONFIG(tooltip)
        cb_useEstimatedMetadata->setToolTip(QCoreApplication::translate("QgsPgNewConnectionBase", "<html>\n"
"<body>\n"
"<p><b>Use estimated table statistics for the layer metadata.</b></p>\n"
"<p>When the layer is setup various metadata is required for the PostGIS table. This includes information such as the table row count, geometry type and spatial extents of the data in the geometry column. If the table contains a large number of rows determining this metadata is time consuming.</p>\n"
"<p>By activating this option the following fast table metadata operations are done:</p>\n"
"<p>1) Row count is determined from results of running the PostgreSQL Analyze function on the table.</p>\n"
"<p>2) Table extents are always determined with the estimated_extent PostGIS function even if a layer filter is applied.</p>\n"
"<p>3) If the table geometry type is unknown and is not exclusively taken from the geometry_columns table, then it is determined from the first 100 non-null geometry rows in the table.</p>\n"
"</body>\n"
"</html>", nullptr));
#endif // QT_CONFIG(tooltip)
        cb_useEstimatedMetadata->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Use estimated table metadata", nullptr));
#if QT_CONFIG(tooltip)
        cb_publicSchemaOnly->setToolTip(QCoreApplication::translate("QgsPgNewConnectionBase", "When searching for spatial tables that are not in the geometry_columns tables, restrict the search to tables that are in the public schema (for some databases this can save lots of time)", nullptr));
#endif // QT_CONFIG(tooltip)
        cb_publicSchemaOnly->setText(QCoreApplication::translate("QgsPgNewConnectionBase", "Only look in the 'public' schema", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QgsPgNewConnectionBase: public Ui_QgsPgNewConnectionBase {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QGSPGNEWCONNECTIONBASE_H
