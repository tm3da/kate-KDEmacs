#include "kdemacsplugin.h"
#include "kdemacsview.h"
#include "../kate/ktexteditor/smartcursor.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KAction>
#include <KActionCollection>
#include <KDialog>
#include <QKeyEvent>

K_PLUGIN_FACTORY (KDEmacsPluginFactory, registerPlugin<KDEmacsPlugin> ("ktexteditor_kdemacs");)
K_EXPORT_PLUGIN (KDEmacsPluginFactory ("ktexteditor_kdemacs", "ktexteditor_plugins"))

const int KILL_RING_MAX_ENTRIES = 100;
QStringList* KDEmacsView::s_killRing = 0;
QStringList::const_iterator KDEmacsView::s_killRing_iterator;

KDEmacsPlugin::KDEmacsPlugin (QObject* parent, const QVariantList& args)
    : KTextEditor::Plugin (parent)
{
    Q_UNUSED (args);
}

KDEmacsPlugin::~KDEmacsPlugin()
{
}

void KDEmacsPlugin::addView (KTextEditor::View* view)
{
    KDEmacsView* nview = new KDEmacsView (view);
    m_views.append (nview);
}

void KDEmacsPlugin::removeView (KTextEditor::View* view)
{
    for (int z = 0; z < m_views.size(); z++) {
        if (m_views.at (z)->parentClient() == view) {
            KDEmacsView* nview = m_views.at (z);
            m_views.removeAll (nview);
            delete nview;
        }
    }
}

void KDEmacsPlugin::readConfig()
{
}

void KDEmacsPlugin::writeConfig()
{
}

KDEmacsView::KDEmacsView (KTextEditor::View* view)
    : QObject (view)
    , KXMLGUIClient (view)
    , m_view (view)
    , m_previousMark (KTextEditor::Cursor::invalid())
    , m_blockInsert (false)

{
    if (!s_killRing)
        s_killRing = new QStringList;

    setComponentData (KDEmacsPluginFactory::componentData());

    connect (m_view, SIGNAL (cursorPositionChanged (KTextEditor::View*, KTextEditor::Cursor)),
             this, SLOT (cursorPositionChangedHandler (KTextEditor::View*, KTextEditor::Cursor)));

    connect (m_view->document(), SIGNAL (textChanged (KTextEditor::Document*)),
             this, SLOT (textChangedHandler (KTextEditor::Document*)));

    connect (m_view, SIGNAL (textInserted (KTextEditor::View*, KTextEditor::Cursor, QString)),
             this, SLOT (textInsertedHandler (KTextEditor::View*, KTextEditor::Cursor, QString)));


    KAction* ac_setMark = new KAction (i18n ("Set Mark"), this);
    actionCollection()->addAction ("tools_kdemacs_set_mark", ac_setMark);
    ac_setMark->setShortcut (Qt::CTRL + Qt::Key_Space);
    connect (ac_setMark, SIGNAL (triggered()), this, SLOT (setMark()));

    KAction* ac_gotoLineStart = new KAction (i18n ("Go to start of line"), this);
    actionCollection()->addAction ("tools_kdemacs_goto_start_line", ac_gotoLineStart);
    ac_gotoLineStart->setShortcut (Qt::CTRL + Qt::Key_A);
    connect (ac_gotoLineStart, SIGNAL (triggered()), this, SLOT (gotoLineStart()));

    KAction* ac_gotoLineEnd = new KAction (i18n ("Go to end of line"), this);
    actionCollection()->addAction ("tools_kdemacs_goto_end_line", ac_gotoLineEnd);
    ac_gotoLineEnd->setShortcut (Qt::CTRL + Qt::Key_E);
    connect (ac_gotoLineEnd, SIGNAL (triggered()), this, SLOT (gotoLineEnd()));

    KAction* ac_killLine = new KAction (i18n ("Kill Line"), this);
    actionCollection()->addAction ("tools_kdemacs_kill_line", ac_killLine);
    ac_killLine->setShortcut (Qt::CTRL + Qt::Key_K);
    connect (ac_killLine, SIGNAL (triggered()), this, SLOT (killLine()));

    KAction* ac_killRegion = new KAction (i18n ("Kill Region"), this);
    actionCollection()->addAction ("tools_kdemacs_kill_region", ac_killRegion);
    ac_killRegion->setShortcut (Qt::CTRL + Qt::Key_W);
    connect (ac_killRegion, SIGNAL (triggered()), this, SLOT (killRegion()));

    KAction* ac_copyRegion = new KAction (i18n ("Copy Region"), this);
    actionCollection()->addAction ("tools_kdemacs_copy_region", ac_copyRegion);
    ac_copyRegion->setShortcut (Qt::ALT + Qt::Key_W);
    connect (ac_copyRegion, SIGNAL (triggered()), this, SLOT (copyRegion()));

    KAction* ac_yank = new KAction (i18n ("Yank"), this);
    actionCollection()->addAction ("tools_kdemacs_yank", ac_yank);
    ac_yank->setShortcut (Qt::CTRL + Qt::Key_Y);
    connect (ac_yank, SIGNAL (triggered()), this, SLOT (yank()));

    KAction* ac_browseKillRingForward = new KAction (i18n ("Browse Kill Ring Forward"), this);
    actionCollection()->addAction ("tools_kdemacs_browse_kill_ring_forward", ac_browseKillRingForward);
    ac_browseKillRingForward->setShortcut (Qt::ALT + Qt::Key_Y);
    connect (ac_browseKillRingForward, SIGNAL (triggered()), this, SLOT (browseKillRingForward()));

    KAction* ac_browseKillRingBackward = new KAction (i18n ("Browse Kill Ring Backward"), this);
    actionCollection()->addAction ("tools_kdemacs_browse_kill_ring_backward", ac_browseKillRingBackward);
    ac_browseKillRingBackward->setShortcut (Qt::ALT + Qt::SHIFT + Qt::Key_Y);
    connect (ac_browseKillRingBackward, SIGNAL (triggered()), this, SLOT (browseKillRingBackward()));

    KAction* ac_rectInsert = new KAction (i18n ("Insert Block"), this);
    actionCollection()->addAction ("tools_kdemacs_rect_insert", ac_rectInsert);
    ac_rectInsert->setShortcut (tr ("Ctrl+X, R, T"));
    connect (ac_rectInsert, SIGNAL (triggered()), this, SLOT (rectInsert()));

    KAction* ac_rectDelete = new KAction (i18n ("Delete Block"), this);
    actionCollection()->addAction ("tools_kdemacs_rect_delete", ac_rectDelete);
    ac_rectDelete->setShortcut (tr ("Ctrl+X, R, D"));
    connect (ac_rectDelete, SIGNAL (triggered()), this, SLOT (rectDelete()));

    KAction* ac_rectKill = new KAction (i18n ("Kill Block"), this);
    actionCollection()->addAction ("tools_kdemacs_rect_kill", ac_rectKill);
    ac_rectKill->setShortcut (tr ("Ctrl+X, R, K"));
    connect (ac_rectKill, SIGNAL (triggered()), this, SLOT (rectKill()));

    KAction* ac_rectYank = new KAction (i18n ("Yank Block"), this);
    actionCollection()->addAction ("tools_kdemacs_rect_yank", ac_rectYank);
    ac_rectYank->setShortcut (tr ("Ctrl+X, R, Y"));
    connect (ac_rectYank, SIGNAL (triggered()), this, SLOT (rectYank()));

    KAction* ac_generalStop = new KAction (i18n ("C-g"), this);
    actionCollection()->addAction ("tools_kdemacs_general_stop", ac_generalStop);
    ac_generalStop->setShortcut (tr ("Ctrl+G"));
    connect (ac_generalStop, SIGNAL (triggered()), this, SLOT (generalStop()));

    setXMLFile ("kdemacsui.rc");
}

KDEmacsView::~KDEmacsView()
{
}

void KDEmacsView::cursorPositionChangedHandler (KTextEditor::View* view, KTextEditor::Cursor cursor)
{
    if (m_previousMark.isValid()) {
        view->setSelection (KTextEditor::Range (m_previousMark, cursor));
    }
    if (m_killRing_range.isValid() && cursor != m_killRing_range.end()) {
        m_killRing_range = KTextEditor::Range::invalid();
    }
}

void KDEmacsView::textChangedHandler (KTextEditor::Document* document)
{
    Q_UNUSED (document)
    if (m_previousMark.isValid()) {
        m_previousMark = KTextEditor::Cursor::invalid();
        m_view->removeSelection();
    }
}

void KDEmacsView::textInsertedHandler (KTextEditor::View* view, KTextEditor::Cursor cursor, QString str)
{
    if (m_blockInsert) {
        view->document()->startEditing();
        view->setBlockSelection (false);
        for (int i = m_startBlockInsertRow;
                i <= m_stopBlockInsertRow; i++) {
            if (cursor.line() != i)
                view->document()->insertText (KTextEditor::Cursor (i, cursor.column()), str, false);
        }
        view->document()->endEditing();
    }
}


void KDEmacsView::keyPressEvent (QKeyEvent* e)
{
  qDebug()<<"keyPressEvent ("<<e<<")";
//     if (m_blockInsert) {
//         m_view->document()->startEditing();
//         m_view->setBlockSelection (false);
// 	int col = m_view->cursorPosition().column();
// 	int row = m_view->cursorPosition().line();
//         for (int i = m_startBlockInsertRow;
//                 i <= m_stopBlockInsertRow; i++) {
// 		m_view->setCursorPosition(KTextEditor::Cursor(row,col));
// 		keyPressEvent(e);                
//         }
//         m_view->document()->endEditing();
//     }
}


void KDEmacsView::setMark()
{
    if (m_previousMark.isValid()) {
        if (m_view->cursorPosition() == m_previousMark) {
            m_previousMark = KTextEditor::Cursor::invalid();
            return;
        }
        m_view->setSelection (KTextEditor::Range (m_previousMark, m_view->cursorPosition()));
    }
    m_previousMark = m_view->cursorPosition();
}

void KDEmacsView::gotoLineStart()
{
    KTextEditor::Cursor cur = m_view->cursorPosition();
    cur.setColumn (0);
    m_view->setCursorPosition (cur);
}

void KDEmacsView::gotoLineEnd()
{
    m_view->setCursorPosition (m_view->document()->endOfLine (m_view->cursorPosition().line()));
}

void KDEmacsView::killLine()
{
    int line = m_view->cursorPosition().line();
    m_view->document()->startEditing();
    m_view->setSelection (KTextEditor::Range (m_view->cursorPosition(), m_view->document()->endOfLine (line)));
    if (m_view->selectionRange().isEmpty()) {
        m_view->setSelection (KTextEditor::Range (KTextEditor::Cursor (line, 0), m_view->cursorPosition()));
        QString str = m_view->selectionText();
        m_view->removeSelection();
        m_view->document()->removeLine (line);
        m_view->insertText (str);
        s_killRing->front().append ("\n");
    } else {
        s_killRing->prepend (m_view->selectionText());
        if ( (s_killRing->size() >= KILL_RING_MAX_ENTRIES) && (!s_killRing->isEmpty()))
            s_killRing->pop_back();
        s_killRing_iterator = s_killRing->constBegin();
        m_view->removeSelectionText();
    }
    m_previousMark = KTextEditor::Cursor::invalid();
    m_view->document()->endEditing();
}

void KDEmacsView::killRegion()
{
    m_view->document()->startEditing();
    s_killRing->prepend (m_view->selectionText());
    if ( (s_killRing->size() >= KILL_RING_MAX_ENTRIES) && (!s_killRing->isEmpty()))
        s_killRing->pop_back();
    s_killRing_iterator = s_killRing->constBegin();
    m_view->removeSelectionText();
    m_previousMark = KTextEditor::Cursor::invalid();
    m_view->document()->endEditing();
}

void KDEmacsView::copyRegion()
{
    m_view->document()->startEditing();
    s_killRing->prepend (m_view->selectionText());
    if ( (s_killRing->size() >= KILL_RING_MAX_ENTRIES) && (!s_killRing->isEmpty()))
        s_killRing->pop_back();
    s_killRing_iterator = s_killRing->constBegin();
    m_view->removeSelection();
    m_previousMark = KTextEditor::Cursor::invalid();
    m_view->document()->endEditing();
}

void KDEmacsView::yank()
{
    if (!s_killRing->isEmpty()) {
        m_view->document()->startEditing();
        KTextEditor::Cursor start = m_view->cursorPosition();
        m_view->insertText (*s_killRing_iterator);
        KTextEditor::Cursor end = m_view->cursorPosition();
        m_killRing_range = KTextEditor::Range (start, end);
        m_view->document()->endEditing();
    }
}

void KDEmacsView::browseKillRing (bool forward)
{
    if (s_killRing_iterator != s_killRing->end()) {
        if (forward) {
            s_killRing_iterator++;
            if (s_killRing_iterator == s_killRing->end())
                s_killRing_iterator = s_killRing->begin();
        } else {
            if (s_killRing_iterator == s_killRing->begin())
                s_killRing_iterator = s_killRing->constEnd();
            s_killRing_iterator--;
        }
        if (m_killRing_range.isValid() && s_killRing_iterator != s_killRing->end()) {
            m_view->document()->startEditing();
            m_view->setSelection (m_killRing_range);
            m_view->removeSelectionText();
            m_view->setCursorPosition (m_killRing_range.start());

            KTextEditor::Cursor start = m_view->cursorPosition();

            m_view->document()->insertText (m_killRing_range.start(), *s_killRing_iterator, m_view->blockSelection());

            KTextEditor::Cursor end = m_view->cursorPosition();
            m_killRing_range = KTextEditor::Range (start, end);

            m_view->document()->endEditing();
        }
    }
}

void KDEmacsView::browseKillRingForward()
{
    browseKillRing (true);
}

void KDEmacsView::browseKillRingBackward()
{
    browseKillRing (false);
}

void KDEmacsView::rectInsert()
{
    m_blockInsert = true;
    m_startBlockInsertRow = m_view->selectionRange().start().line();
    m_stopBlockInsertRow = m_view->selectionRange().end().line();
    m_view->setBlockSelection (true);  
}

void KDEmacsView::rectDelete()
{
    m_view->setBlockSelection (true);
    m_view->removeSelectionText();
    m_view->setBlockSelection (false);
}

void KDEmacsView::rectKill()
{
    m_view->setBlockSelection (true);
    killRegion();
    m_view->setBlockSelection (false);
}

void KDEmacsView::rectYank()
{
    m_view->setBlockSelection (true);
    yank();
    m_view->setBlockSelection (false);
}

void KDEmacsView::generalStop()
{
    m_blockInsert = false;
    m_previousMark = KTextEditor::Cursor::invalid();
    m_view->setBlockSelection (false);
}


#include "kdemacsview.moc"

















