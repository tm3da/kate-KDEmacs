#ifndef KDEMACSPLUGIN_H
#define KDEMACSPLUGIN_H

#include <KTextEditor/Plugin>

namespace KTextEditor
{
class View;
}

class KDEmacsView;

class KDEmacsPlugin
    : public KTextEditor::Plugin
{
public:
    // Constructor
    explicit KDEmacsPlugin (QObject* parent = 0, const QVariantList& args = QVariantList());
    // Destructor
    virtual ~KDEmacsPlugin();

    void addView (KTextEditor::View* view);
    void removeView (KTextEditor::View* view);

    void readConfig();
    void writeConfig();

//     void readConfig (KConfig *);
//     void writeConfig (KConfig *);

private:
    QList<class KDEmacsView*> m_views;
};

#endif
