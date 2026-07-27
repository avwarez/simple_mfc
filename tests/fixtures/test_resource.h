// test_resource.h — symbol header for the .rc compiler test fixture.
// Deliberately mixes decimal, hex and a "+"-expression id, as real
// resource.h files do, to exercise ParseSymbolHeader.
#define IDD_SAMPLE           1000
#define IDC_NAME_EDIT        1001
#define IDC_GO_BUTTON        1002
#define IDC_ENABLE_CHECK     0x03EB   /* == 1003 */
#define IDC_OPTION_A         1004
#define IDC_OPTION_B         (IDC_OPTION_A + 1)
#define IDC_FILE_LIST        1006
#define IDC_MODE_COMBO       1007
#define IDC_PROGRESS_BAR     1008
#define IDC_GROUP            1009
#define IDC_STATIC           -1
