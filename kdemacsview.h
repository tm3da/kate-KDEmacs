#ifndef KDEMACSVIEW_H
#define KDEMACSVIEW_H

#include <QObject>
#include <QKeyEvent>
#include <KXMLGUIClient>
#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>
#include "kdemacsplugin.h"

class KDEmacsView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit KDEmacsView (KTextEditor::View* view = 0);
    ~KDEmacsView();

public slots:
    void setMark();
    void gotoLineStart();
    void gotoLineEnd();
    void killLine();
    void killRegion();
    void copyRegion();
    void yank();
    void browseKillRingForward();
    void browseKillRingBackward();
    void rectInsert(); //C-x r t
    void rectDelete(); //C-x r d
    void rectKill(); //C-x r k
    void rectYank(); //C-x r y
    void generalStop(); //C-g
protected:
    void browseKillRing (bool forward);

protected slots:
    void cursorPositionChangedHandler (KTextEditor::View* view, KTextEditor::Cursor cursor);
    void textChangedHandler (KTextEditor::Document* document);
    void textInsertedHandler (KTextEditor::View* view, KTextEditor::Cursor cursor, QString str);

private:
    KTextEditor::View* m_view;
    KTextEditor::Cursor m_previousMark;
    static QStringList::const_iterator s_killRing_iterator;
    KTextEditor::Range m_killRing_range;
    static QStringList * s_killRing;
    bool m_blockInsert;
    int  m_startBlockInsertRow;
    int  m_stopBlockInsertRow;
    
};

#endif
