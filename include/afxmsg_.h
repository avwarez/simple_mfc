// afxmsg_.h — reference STUB (declarations only, no implementation).
// Message-map "entry" macros (they map a message/command/notification to
// a member function). Per Microsoft Learn, the required header for
// ON_COMMAND/ON_MESSAGE/ON_REGISTERED_MESSAGE/ON_CONTROL is afxmsg_.h
// (the demarcation macros BEGIN/END/DECLARE_MESSAGE_MAP instead stay in
// afxwin.h, where real MFC puts them). Included by afxwin.h itself, as in
// real MFC where a single #include <afxwin.h> also exposes these macros.
#pragma once

#define ON_COMMAND(commandId, memberFxn)
#define ON_MESSAGE(message, memberFxn)              // handler: afx_msg LRESULT memberFxn(WPARAM, LPARAM);
#define ON_REGISTERED_MESSAGE(nMessageVariable, memberFxn)
#define ON_CONTROL(wNotifyCode, commandId, memberFxn)

// ON_NOTIFY has no dedicated "Requirements" page on Microsoft Learn (it is
// absent from the current message-map-macros-mfc table), but conceptually
// belongs to the same family as ON_CONTROL/ON_COMMAND (a mapping macro,
// not a demarcation one) — placed here by pattern, not as a direct
// citation of a primary source.
#define ON_NOTIFY(wNotifyCode, id, memberFxn)        // handler: afx_msg void memberFxn(NMHDR*, LRESULT*);

// Convenience wrappers around ON_CONTROL — not individually documented by
// Microsoft, inferred by pattern from the afxmsg_.h source.
#define ON_BN_CLICKED(id, memberFxn)      ON_CONTROL(/*BN_CLICKED*/ 0, id, memberFxn)
#define ON_EN_CHANGE(id, memberFxn)       ON_CONTROL(/*EN_CHANGE*/ 0, id, memberFxn)
#define ON_CBN_SELCHANGE(id, memberFxn)   ON_CONTROL(/*CBN_SELCHANGE*/ 0, id, memberFxn)
#define ON_STN_CLICKED(id, memberFxn)     ON_CONTROL(/*STN_CLICKED*/ 0, id, memberFxn)

// ON_UPDATE_COMMAND_UI is intentionally NOT declared here: it is not part
// of the real-world usage this subset targets. Note: Microsoft Learn
// documents it with Requirements: Header afxole.h, not afxmsg_.h.
