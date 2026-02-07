#ifndef I18N_H
#define I18N_H

#include <QString>

namespace I18n {

void setLanguage(const QString &lang);
QString language();
QString tr(const QString &key);

}

#endif // I18N_H
