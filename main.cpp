#include <QtCore/QCoreApplication>

// ========================================================================== //
//
// ========================================================================== //
int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("AlexisSoft");
    QCoreApplication::setOrganizationDomain("free-lance.ru");
    QCoreApplication::setApplicationName("stratumon");

    QCoreApplication app(argc, argv);

    return app.exec();
}
