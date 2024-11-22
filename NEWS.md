Red Panda C++ Version 2.6

  - enhancement: Highlighter for makefiles

Red Panda C++ Version 2.5

  - enhancement: New color scheme Monokai (contributed by 小龙Dev(XiaoLoong@github))
  - enhancemnet: Add "Reserve word for Types" item in color scheme
  - enhancement: Auto save / load problem set
  - enhancement: Project's custom compile include/lib/bin directory is under folder of the app, save them using the path relative to the app
  - enhancement: Slightly reduce memory usage   
  - enhancement: Options -> editor -> custom C/C++ type keywords page
  - change: Default value of option "Editors share one code analyzer" is ON
  - change: Default value of option "Auto clear symbols in hidden editors" is OFF
  - enhancement: Show completion suggest for "namespace" after "using"
  - fix: MinGW-w64 gcc displayed as "MinGW GCC"
  - enhancement: Deduce type info for "auto" in some simple cases for stl containers.
  - fix: Crash when no semicolon or left brace after the keyword "namespace"
  - fix: Can't correctly show completion suggest for type with template parameters
  - enhancement: Show compltion suggest for std::pair::first and std::pair second
  - enhancement: Disable "run" and "debug" actions when current project is static or dynamic library
  - enhancement: Add "Generate Assembly" in "Run" Menu
  - enhancement: Improve highlighter for asm
  - enhancement: Use asm highlighter in cpu window
  - fix: "AT&T" radio button not correctly checked in cpu window 
  - enhancement: Remove blank lines in the register list of cpu window.
  - fix: Cpu window's size not correctly saved, if it is not closed before app exits.
  - fix: Can't restore cpu window's splitter position.

Red Panda C++ Version 2.4

  - fix: Contents in class browser not correctly updated when close the last editor for project. 
  - fix: When all editors closed, switch browser mode dosen't correct update the class browser;
  - fix: "check when open/save" and "check when caret line changed" in Options Dialog / Editor / Syntax Check don't work
  - fix: Crash when editing a function at the end of file without ; or {
  - enhancement: Add the "parsing TODOs" option in Options Dialog / Editor / Misc
  - enhancement: Remove todos/bookmarks/breakpoints when deleting file from project
  - enhancement: Rename filenames in todos/bookmarks/breakpoints  when renaming project file
  - enhancement: Rename filenames in bookmarks/breakpoints after a file is save-ased.
  - fix: Can't goto definition of classes and namespaces displayed in the class browser on whole project mode.
  - fix: macro defines parsed before not correctly applied in the succeeding parse.
  - fix: function pointers not correctly handle in code parser;
  - fix: var assignment not correctly handled in code parser;
  - fix: function args not correctly handled in code parser;
  - fix: crash when alt+mouse drag selection
  - enhancement: show completion tips for when define a function that already has a declaration.
  - enhancement: Use relative paths to save project settings
  - fix: Layout for project options dialog's general page is not correct.
  - fix: modifitions in the project options dialogs's dll host page is not correctly saved.
  - enhancement: In the project options dialog, autoset the default folder in the openning dialog when choosing file/directory paths.
  - fix: Escape suquences like \uxxxx and \Uxxxxxxxx in strings are not correctly highlighted.
  - enhancement: Search / replace dialogs redesigned.
  - fix: inline functions are not correctly parsed;
  - fix: &operator= functions are not correctly parsed;
  - fix: Code Formatter's "add indent to continueous lines" option is not correctly saved.
  - fix: _Pragma is not correctly handled;
  - enhancement: improve parse result for STL <random>
  - change:  the default value for UI font size : 11
  - change:  the default value for add leading zeros to line numbers : false
  - upgrade integrated rturtle. fix: nothing is drawed when set background color to BLACK
  - upgrade integrate fmtlib. fix: imcompatible with GBK encoding

Red Panda C++ Version 2.3

  - fix: When start parsing and exit app, app may crash
  - enhancement: add "Allow parallel build" option in project option dialog's custom compile options page
  - fix: crash when rename project file
  - fix: When remove project file, symbols in it not correctly removed from code parser
  - fix: infos in class browser (structure panel) not correctly updated when add/create/remove/rename project files


Red Panda C++ Version 2.2

  - enhancement: basic code completion support for C++ lambdas
  - enhancement: slightly reduce parsing time
  - fix: Wrong charset name returned when saving file
  - fix: 'using =' / 'namespace =' not correctly handled
  - fix: Pressing '*' at begin of line will crash app
  - enhancement: switch header/source in editor's context menu
  - enhancement: base class dropdown list in new class dialog now works 
  - fix: Edting / show context menu when code analyzer is turned off may crash app.
  - fix: Show context menu when edting non c/c++ file may crash app.
  - fix: Project Options Dialog's Files panel will crash app.
  - fix: Memory usage of undo system is not correctly calculated, which may cause undo items lost
  - fix: Set max undo memory usage to 0 don't really remove the limit for undo
  - fix: Set max undo times to 0 don't really remove the limit for undo
  - fix: Keep the newest undo info regardless of undo memory usage
  - fix: inline functions not correctly handled by parser
  - fix: &operator= not correctly handled by parser

Red Panda C++ Version 2.1

  - fix: editors that not in the editing panel shouldn't trigger switch breakpoint
  - fix: editors that not in the editing panel shouldn't show context menu
  - enhancement: add "editors share one code parser" in "options" / "editor" / "code completion", to reduce memory usage.
        Turned off by default on PCs with memory > 4G; Force turned on PCs with memory < 1G.
  - enhancement: add "goto block start"/"goto block end" in "Code" menu
  - add fmtlib to the gcc compiler's lib distributed with RedPanda IDE windows version
  - add default autolink for fmtlib in Windows 
  - reduce size of the executable of win-git-askpass tool
  - change: remove "Optimize for the following machine" and "Optimize less, while maintaining full compatibility" options in the compiler setting panel, which are obseleted.
  - change: escape spaces in the executabe path under linux.
  - fix: Before run  a project's executable, we should check timestamp for project files AND modification states of files openned in editor.
  - change: Don't turn on "Show some more warnings (-Wextra)" option by default for DEBUG compiler set 
  - fix: Changes mainwindows's compiler set combobox not correctly handled for project
  - change: Don't localize autogenerated name for new files and new project (new msys2 gcc compiler can't correctly handle non-ascii chars in filenames)
  - change: rename "file" Menu -> "New Source File" to "New File" 

Red Panda C++ Version 2.0

  - redesign the project parser, more efficient and correct
  - enhancement: todo parser for project
  - fix: save/load bookmark doesn't work
  - fix: if project has custom makefile but not enabled, project won't auto generate makefile.
  - fix: File path of Issues in project compilation is relative, and can't be correctly marked in the editors.
  - fix: editor & class browser not correct updated when editor is switched but not focused
  - enhancement: show all project statements in the class browser
  - fix: namespace members defined in multiple places not correctly merged in the class browser
  - fix: correctly display statements whose parent is not in the current file
  - fix: statements is the class browser is correctly sorted
  - enhancement: Weither double click on the class browser should goto definition/declaration,  depends on the current cursor position
  - enhancement: keep current position in the class browser after contents modified
  - fix: "." and ".." in included header paths not correctly handled
  - reduce memory usage when deciding file types
  - enhancement: refresh project view for git status won't redraw project structure
  - enhancement: auto save project options after the compilerset option for project resetted 
  - enhancement: "." and ".." in paths of issues not correctly handled
  - enhancement: auto locate the last opened file in the project view after project creation
  - enhancement: separate compiler's language standard option for C / C++
  - fix: compiler settings not correctly handled when create makefile
  - enhancement: auto locate current open file in the project view panel
  - enhancement: when closing project, prevent all editors that belongs to the project check syntax and parse todos.
  - enhancement: add "auto reformat when saving codes" in "Options" / "Editor" / "Misc" (off by default)
  - enhancement: use "todo" and "fixme" as the keyword for TODO comments
  - fix: rules for obj missing in the makefile generated for project 
  - enhancement: before run a project'executable, check if there's project file  newer than the executable
  - enhancement: when create a new folder in the files view, auto select that folder and rename it
  - enhancement: when new header in the project view, auto select basename in the filename dialog
  - enhancement: when add file in the project view, auto select basename in the filename dialog
  - change: Don't generate localized filename when new header/add file in the project view
  - fix: Restore project's original compiler set if user choose 'No' in the confirm project compiler set change dialog.
  - fix: Encoding info in the status bar not correctly updated when save a new file
  - enhancement: auto sort TODO items 
  - fix: Correctly set file's real encoding to ASCII after saving
  - fix: selection's position not correctly set after input a char / insert string (and causes error under OVERWRITE mode)
  - fix: editors that not in the editing panel should not be syntax checked/ todo parsed/ code analyzed
  - fix: editors that not in the editing panel should not trigger breakpoint/bookmark/watch switch

Red Panda C++ Version 1.5

  - fix: project files that lies in project include folder is wrongly openned in Read-only mode
  - enhancement: add/new/remove/rename project files won't rebuild project tree
  - fix: gliches in UI's left panel in some OS
  - fix: correctly restore project layout when reopen it
  - change: new syntax for project layout files
  - change: clear tools output panel when start to compile
  - change: don't show syntax check messages in the tools output panel (to reduce longtime memory usage)
  - fix: minor memory leaks when set itemmodels
  - fix: threads for code parsing doesn't correctly released when parsing finished ( so and the parsers they use)
  - enhancement: save project's bookmark in it's own bookmark file
  - enhancement: project and non-project files use different bookmark view (auto switch when switch editors)
  - enhancement: auto merge when save bookmarks.
  - enhancement: add option "max undo memory usage" in the options / editor / misc page
  - fix: icons in options dialogs  not correctly updated when change icon set
  - enhancement: set compilation stage in the options / compiler set pages
  - enhancement: set custom compilation output suffix in the options / compiler set pages
  - enhancement: project and non-project files use different breakpoint and watchvar view (auto switch when not debugging and editor switched)
  - enhancement: save project's breakpoint and watchvar in it's own debug file.
  - enhancement: delete a watch expression don't reload who watch var view
  - enhancement: auto save/restore debug panel's current tab
  - fix: correctly restore left(explorer) panel's current tab
  - enhancement: auto close non-modified new editor after file/project openned;
  - fix: project files openned by double click in bookmark/breakpoint panel may cause app crash when closed.
  - fix: When open a project that's already openned, shouldn't close it.
  - enhancement: When open a project, let user choose weither open it in new window or replace the already openned project
  - fix: editor tooltip for #include_next is not correctly calculated
  - fix: ctrl+click on #include_next header name doesn't open the right file
  - enhancement: parser used for non-project C files won't search header files in C++ include folders.
  - fix: toggle block comment/delete to word begin/delete to word end are not correctly disabled when editor not open
  - fix: index out of range in cpp highlighter
  - fix: memory leak in code folding processing
  - change: add/remove/new project file won't save all openned project files.
  - fix: save all project files shouldn't trigger syntax check in inactive editors 

Red Panda C++ Version 1.4

  - fix: "Encode in UTF-8" is not correctly checked, when the editor is openned using UTF-8 encoding.
  - fix: crash when create non C/C++ source file in project
  - fix: can't open text project file in the editor
  - change: when create non-text project file, don't auto open it
  - fix: the project compiler options is not correctly read  when open old dev-c++ project 
  - fix: astyle.exe can't correctly format files that using non-ascii identifier
  - fix: when "cleary symbol table of hidden editors" is turned on, content in the editor reshown is not correctly parsed

Red Panda C++ Version 1.3

  - enhancement: don't parse all openned files when start up
  - enhancement: don't parse files when close all and exit
  - change: reduce time intervals for selection by mouse
  - enhancement: change orders of the problems in the problem set panel by drag&drop
  - enhancement: change orders of the problem cases in the problem panel by drag&drop
  - fix: the size of horizontal caret is wrong

Red Panda C++ Version 1.2

  - enhancement: Portuguese Translation ( Thanks for crcpucmg@github)
  - fix: files in network drive is opened in readonly mode
  - change: reorganization templates in subfolders
  - enhancement: create template from project
  - fix: can't correctly set project icon
  - fix: can't set shortcut that contains shift and non-alphabet characters

Red Panda C++ Version 1.1.6

  - fix: block indent doesn't work
  - fix: selection is not correctly set after input in column mode 
  - fix: in #if, defined without () not correctly processed
  - enhancement: don't show cpp defines when editing c files
  - enhancement: choose default language when first run
  - fix: Drag&Drop no correctly disabled for readonly editors
  - enhancement: disable column mode in readonly editors
  - fix: inefficient loop when render long lines
  - fix: indents for "default" are not the same with "case"
  - fix: (wrongly) use the default font to calculate  non-ascii characters' width
  - change: switch positions of problem case output and expected output

Red Panda C++ Version 1.1.5

  - change: uncheck "hide unsupported files" in files view shouldn't gray out non-c files
  - enhancement: double clicking a non-text file in the files view, will open it with external program
  - enhancement: double clicking a non-text file in the project's view, will open it with external program
  - fix: correctly update the start postion of selection after code completion
  - enhancement: add a demo template for raylib/rdrawing predefined colors
  - enhancement: add select current word command in the Selection menu
  - change: add Selection menu
  - enhancement: add memory view rows/columns settings in the settings dialog -> debugger -> general panel
  - enhancement: add "Go to Line..." in the Code menu
  - fix: "Timeout for problem case" can't be rechecked, in the Settings Dialog -> executor -> problem set panel.
  - fix: bug in the project template
  - change: sort local identifiers before keywords in the auto completion popup
  - fix: can't create folder in files view, if nothing is selected
  - fix: can't find the gcc compiler, if there are gcc and clang compilers in the same folder

Red Panda C++ Version 1.1.4

  - enhancement: prohibit move selection up/down under column mode
  - enhancement: prohibit move selection up/down when the last line in selection is a folded code blocks
  - enhancement: check validity of selection in column mode when moving caret by keyboard
  - enhancement: check validity of selection in column mode when moving caret by mouse
  - enhancement: only allow insert linebreak at the end of folded code block
  - enhancement: only allow delete whole folded code block
  - refactor of undo system
  - fix: calculation of the code block ranges when inserting/deleting
  - fix: undo chains
  - enhancement: prevent group undo when caret position changed
  - fix: undo link break may lose leading spaces
  - fix: correctly restore editor's modified status when undo/redo
  - enhancement: set current index to the folder after new folder created in the files view
  - enhancement: resort files in the files view after rename

Red Panda C++ Version 1.1.3

  - fix: wrong auto indent calculation for comments
  - enhancement: position caret at end of the line of folded code block
  - enhancement: copy the whole folded code block
  - enhancement: delete the whole folded code block
  - fix: correctly update the folding state of code block, when deleted
  - change: just show one function hint for overloaded functions
  - update raylib to 4.2-dev
  - update raylib-drawing to 1.1
  - add "raylib manual" in the help menu

Red Panda C++ Version 1.1.2
  - enhancement: use different color to differenciate folder and headers in completion popup window
  - enhancement: auto add "/" to folder when completing #include headers
  - enhancement: add the option "Set Encoding for the Executable" to project's compiler options
  - fix: can't correctly compile when link params are seperated by line breaks
  - fix: select all shouldn't set file's modified flag
  - enhancement: add (return)type info for functions/varaibles/typedefs in the class browser panel
  - enhancement: autolink add "force utf8" property (mainly for raylib)
  - change: position caret to (1,1) when create a new file using editor's new file template

Red Panda C++ Version 1.1.1
  - enhancement: adjust the appearance of problem case's input/output/expected control
  - change: swap position of problem case's output and expected input controls
  - enhancement: when problem case panel is positioned at right, problem case's input, output and expected controls is layouted vertically
  - enhancement: add ignore spaces checkbox in problem cases panel
  - fix: can't paste contents copied from Clion/IDEA/PyCharm
  - fix: project don't have compiler set bin folder setting
  - fix: when run/debug the executable, add current compiler set's bin folders to path
  - fix: when open in shell, add current compiler set's bin folders to path
  - fix: when debug the executable using gdb server, add current compiler set's bin folders to path
  - fix: reduce height of the message panel when dragging from right to bottom
  - fix: when messages panel is docked at right, its width not correctly restored when restart.

Red Panda C++ Version 1.1.0
  - enhancement: when ctrl+mouse cursor hovered an identifier or header name, use underline to highlight it
  - enhancement: mark editor as modified, if the editing file is changed by other applications.
  - enhancement: When the editing files is changed by other applications, only show one notification dialog for each file.
  - fix: c files added to a project will be compiled as c++ file.
  - enhancement: restore caret position after batch replace
  - enhancement: rename in files view's context menu
  - enhancement: delete in files view's context menu
  - change: drag&drop in files view default to move
  - fix: rename macro doesn't work in project
  - fix: undo doesn't work correctly after rename symbole & reformat
  - fix: can't remove a shortcut
  - enhancement: hide all menu actions in the option dialog's shortcut panel
  - enhancement: add 'run all problem cases' / 'run current problem case' / 'batch set cases' to the option dialog's shortcut panel
  - enhancement: more templates for raylib
  - fix: compiler settings not correctly saved

Red Panda C++ Version 1.0.10
  - fix: modify watch doesn't work
  - fix: make behavior consistent in adding compiler bindirs to Path (thanks for brokencuph@github)
  - enhancement: basic MacOS support ( thanks for RigoLigoRLC@github)
  - fix: #define followed by tab not correctly parsed
  - enhancement: don't auto add () when completing C++ io manipulators ( std::endl, std::fixed, etc.)
  - fix: can't goto to definition of std::endl
  - fix: errors in the calculation of cut limit
  - enhancement: new turtle library based on raylib ( so it can be used under linux)
  - fix: autolink calculation not stable

Red Panda C++ Version 1.0.9
  - fix: selection in column mode not correctly drawn when has wide chars in it
  - fix: delete & insert in column mode not correctly handled
  - fix: input with ime in column mode not correctly handled
  - fix: copy & paste in column mode not correctly handled
  - fix: crash when project name is selected in the project view and try create new project file
  - change: panels can be relocated
  - fix: tab icon not correct restore when hide and show a panel
  - fix: the hiding state of the tools output panel is not correctly saved
  - enhancement: add "toggle explorer panel" and "toggle messages panel" in "view" menu
  - fix: cursor is wrongly positioned when insert code snippets that don't have placeholders
  - fix: "run current cases" dosen't correctly display real output 

Red Panda C++ Version 1.0.8
  - enhancement: auto complete '#undef'
  - enhancement: redesign components for compiler commandline arguments processing
  - fix: selection calculation error when editing in column mode
  - enhancement: add compiler commandline argument for "-E" (only preprocessing)
  - enhancement: auto set output suffix to ".expanded.cpp" when compiler commandline argument for "-E" is turned on
  - enhancement: auto set output suffix to ".s" when compiler commandline argument for "-S" is turned on
  - enhancement: show error message when user set a shortcut that's already being used.
  - enhancement: adjust scheme colors for "dark" and "high contrast" themes
  - enhancement: can debug files that has non-ascii chars in its path and is compiled by clang
  - fix: when debugging project, default compiler set is wrongly used

Red Panda C++ Version 1.0.7
  - change: use Shift+Enter to break line
  - change: highlight whole #define statement using one color
  - enhancement: don't highlight '\' as error
  - enhancement: hide add charset  option in project options dialog's compiler set page, when project compiler set is clang
  - fix: When generating project's makefile for clang, don't add -fexec-charset / -finput-charset command line options
  - fix: index of the longest line not correctly updated when inputting with auto completion open
  - enhancement: support UTF-8 BOM files
  - enhancement: add new tool button for "compiler options"
  - fix: keyword 'final' in inhertid class definition is not correctly processed
  - change: stop generating 'profile' compiler set

Red Panda C++ Version 1.0.6
  - fix: gcc compiler set name is not correct in Linux
  - enhancement: hide add charset option when the currect compiler set is clang
  - enhancement: auto check the c project option in the new project dialog
  - change: use "app.ico" as default name for the project icon file
  - fix: c file should use CC to build in the auto generated makefile
  - enhancement: package script for msys2 clang
  - enhancement: auto set problem case's expected output file which has "ans" as the suffix, when batch set cases
  - fix: use utf8 as the encoding for clang's error output
  - fix: correctly parse link error message for clang
  
Red Panda C++ Version 1.0.5
  - enhancement: add autolink and project template for sqlite3
  - enhancement: add sqlite3 lib to the gcc in distribution
  - enhancement: improve the matching of function declaration and definitions
  - fix: research button doesn't show find in files dialog
  - enhancement: add project template for libmysqlclient(libmariadbclient)
  - enhancement: add libmysqlclient to the x86-64 version gcc in distribution
  - enhancement: select and delete multiple watches
  - enhancement: add project templates for tcp server / tcp client
  - enhancement: only show function tips when cursor is after ',' or '('.
  - enhancement: when auto complete function names, only append '(' if before identifier or "/'
  - update highconstrast icon set
  - fix: index of the longest line not correctly updated when insert/delete multiple lines ( which will cause selection errors)
  
Red Panda C++ Version 1.0.4
  - fix: hide function tips, when move or resize the main window
  - enhancement: add help link for regular expression in search dialog
  - enhancement: remember current problem set's filename
  - enhancement: F1 shorcut opens offcial website
  - enhancement: don't auto complete '(', if the next non-space char is '(' or ident char
  - enhancement: if a project's unit encoding is the same with project's encoding, don't save its encoding
  - fix: files will be saved to default encoding inspite of its original encoding
  - fix: parenthesis skip doesn't work when editing non-c/c++ files
  - enhancement: prefer local headers over system headers when complete #include header path
  - fix: tab/shift+tab not correctly handled in options dialog's code template page
  - enhancement: batch set cases ( in problem case table's context menu )
  - enhancement: add Portugese translation
  - fix: crash when eval statements like "fsm::stack fsm;"
  - enhancement: add Traditional Chinese translation
  - fix: index of the longest line not correctly updated ( which will cause selection errors)
  - fix: scroll bar not correctly updated when collapse/uncollapse folders
  - fix: parse error for definition of functions whose return type is pointer
  - enhancement: add library in project option dialog's compile options page

Red Panda C++ Version 1.0.3
  - fix: when oj problem grabbed by competitive companion received,
    the app is restored to normal state, no matter it's current state.
  - enhancement: input shortcut in the option dialog's general -> shortcut page by pressing keys.
  - enhancement: shift+ctrl+down/up to move currenlt selection lines up / down
  - fix: can't compile under linux
  - enhancement: support Devcie Pixel Ratio ( for linux )
  - fix: crash when editing txt file and input symbol at the beginning of a line
  - fix: ctrl+shift+end doesn't select
  - fix: don't show tips in the editor, when selecting by mouse
  - fix: auto syntax check doesn't work for new files
  - change: don't auto jump to the first syntax error location when compile
  - enhancement: don't show folders that doesn't contain files in the project view
  - enhancement: redesigned new project unit dialog
  - fix: some dialog's icon not correctly set
  - fix: can't build project that has source files in subfolders
  - fix: can't build project that has custom object folder
  - fix: buttons in the project option dialog's output page don't work
  - fix: don't add non-project header files to makefile's object rules
  - change: add glm library in the integrated gcc

Red Panda C++ Version 1.0.2
  - enhancement: press tab in column mode won't exit column mode
  - enhancement: refine behavior of undo input space char
  - enhancement: better display when input with IM under column mode
  - enhancement: better display current lines under column mode
  - change: test to use utf-8 as the default encoding (prepare to use libclang to implement parser)
  - fix: auto syntax check fail, if the file is not gbk and includes files encoded with utf8
  - enhancement: timeout for problem case test
  - enhancement: slightly reduce start up time
  - enhancement: use icon to indicate missing project files in the project view
  - enhancement: display problem case running time 
  - enhancement: set problem case input/expected output file
  - enhancement: auto position cursor in expected with output's cursor
  - enhancement: display line number in problem case's input/output/expected input controls
  - enhancement: only tag the first inconstantency when running problem case, to greatly reduce compare & display time
  - fix: can't stop a freeze program that has stdin redirected.
  - enhancement: context menu for problem cases table 
  - fix: error in auto generate makefile under linux
  - fix: when open a project, and it's missing compiler set getten reset, it's modification flag is not correctly set.
  - fix: vector vars can't be expanded in the watch panel
  - change: use qt's mingw 8.1 (32bit) and 11.2 (64bit) in distributions, to provide better compatibility with simplified chinese windows.
  - fix: crash when rename an openned file, and choose "no" when ask if keep the editor open
  - change: only auto complete symbol '(' when at line end, or there are spaces or right ')' '}' ']'after it
  - fix: mouse drag may fail when start drag at the right half of the selection's last character

Red Panda C++ Version 1.0.1
  - fix: only convert project icon file when it's filename doesn't end with ".ico"
  - fix: hide function tip when scroll
  - fix: short cut for goto definition/declaration doesn't work
  - enhancement: press alt to switch to column selection mode while selection by mouse dragging in editor
  - fix: order for parameters generated by auto link may not correct
  - fix: corresponding '>' not correctly removed when deleting '<' in #include line 
  - enhancement: shortcut for goto definition/declaration
  - change: ctrl+click symbol will goto definition, instead of got declaration
  - fix: when size of undo items is greater than the limit, old items should be poped in group
  - enhancement: max undo size in option dialog's editor->misc tab
  - fix: when editor font is too small, fold signs on the gutter are not correctly displayed
  - fix: expand fold signs on the gutter are not correct
  - enhancement: auto restore mainwindow when open files in one instance
  - fix: the problem & problem set panel can't be correctly , if problem set is enabled
  - fix: disable code completion doesn't correctly disable project parser
  - enhancement: slightly reduce memory usage for code parser
  - enhancement: switch capslock won't cancel code completion
  - enhancement: double click on item in code completion list will use it to complete
  - fix: goto declaration by ctrl+click will incorrectly select contents
  - fix: input may cause error, if selection in column mode and begin/end at the same column
  - enhancement: draw cursor for column mode
  - enahcnement: edit/delete in multiline ( column mode), press ese to exit

Red Panda C++ Version 1.0.0
  - fix: calculation for code snippets's tab stop positions is not correct
  - fix: Refresh files view shouldn'tchange open/save dialog's default folder
  - enhancement: "locate in files view" will request user's confirmation when change the working folder
  - enhancement: adjust tab order in the find dialog
  - enhancement: highlight hits in the find panel's result list
  - enhancement: optimize startup time
  - fix: batch replace in file doesn't respect item check states in the find panel
  - enhancement: option for default file encoding in "option" dialog's "editor"->"misc" tab
  - enhancement: auto detect "gbk" encoding when running in zh_CN locale under Linux
  - enhancement: disable encoding submenu when editor closed
  - enhancement: clear infos in the status bar when editor closed
  - fix: wrong selection when drag & dropped in editor
  - enhancement: toggle block comment
  - fix: syntax color of #include header filenames not correct
  - enhancement: disable "code completion" will disable enhanced syntax highlight
  - enhancement: match bracket
  - enhancement: **Linux** convert to "gbk"/"gb18030" encodings when run under "zh_CN" locale
  - fix: when no selection, copy/cut should auto select whole line with the line break
  - fix: redo cut with no selection while make whole line selected
  - fix: correctly reset caret when redo cut with no selection
  - enhancement: close editor when middle button clicked on it's title tab
  - fix: error when insert text in column mode 
  - fix: error when delete contents in column mode on lines that has wide-chars
  - fix: error when create folder in files view
  - fix: "ok" button should be disabled when no template selected in new project dialog
  - fix: saveas an openned project file shouldn't be treated as rename
  - enhancement: auto add parentheis when complete function like MARCOs
  - fix: wrong font size of exported RTF file 
  - fix: correct tokenize statements like "using ::memcpy";
  - fix: wrong font size of exported HTML file 
  - fix: parse error in avxintrin.h 
  - fix: switch disassembly mode doesn't update contents
  - fix: if there is a Red Panda C++ process running program, other Red Panda C++ processes can't run program correctly.
  - enhancement: ctrl+enter insert a new line at the end of current line
  - enhancement: create file in files view
  - fix: hits in the search view not correctly displayed (overlapped with others)
  - enhancement: auto convert project icon to ico format
  - fix: correctly reparse modified project files when rename symbol
  - change: remove shortcuts for line/column mode

Red Panda C++ Version 0.14.5
  - fix: the "gnu c++ 20" option in compiler set options is wrong
  - enhancement: option "open files in the same red panda C++ instance", in options->environment->file associations
  - enhancement: hide unsupported files in files view
  - fix: can't correctly set break conditions
  - fix: crash when copy to non-c files
  - fix: fonts in cpu window is not correctly set, when dpi changed
  - enhancement: enable group undo
  - enhancement: add option "hide symbols start with underscore" and "hide synbols start with two underscore"
  - fix: can't rename project files that not openned in editor
  - enhancement: group undo will stop at spaces
  - fix: menu font size is wrong when dpi changed
  - enhancement: better processing of symbol completion
  - enhancement: better support of ligatures
  - enhancement: use the expression evaluation logic to handle "goto declaration"/"goto definition" 
  - enhancement: reduce startup time by about 1 second.
  - enhancement: add option "mouse selection/drag scroll speed" in the options dialog's "Editor" / "general" tab.
  - fix: the scroll speed of mouse selection/drag is too fast.
  - fix: the scroll behavior of mouse dragging on the editor's edge is not correct
  - fix: calculation of caret position is not in consistence.
  - fix: undo one symbol completion as a whole operation
  - fix: crash when open a project that contains custom folder
  - enhancement: symbol completion when editor has selection 
  - fix: save project's layout shouldn't modify the project file
  - enhancement: use expression processing in syntax highlighting for identifiers
  - fix: if a function's declaration can't be found, it will be wrongly highlighted as variable
  - change: "locate in files view" won't change the working folder, if current file is in subfolders of the working folder
  - enhancement: hide function tips, when input method is visible

Red Panda C++ Version 0.14.4
  - enhancement: git - log
  - fix: error in templates
  - enhancement: git - reset
  - fix: header completion error when header name contains '+'
  - enhancement: clear history in file -> recent menu
  - enhancement: close project in project view's context menu
  - enhancement: auto find compiler sets when run for the first time
  - enhancement: git - remotes
  - enhancement: rename "open folder" to "choose working folder"
  - enhancement: let user choose app theme when first run
  - enhancement: git - pull / push / fetch

Red Panda C++ Version 0.14.3
  - fix: wrong code completion font size, when screen dpi changed
  - enhancement: replace Files View Panel's path lineedit control with combo box
  - enhancement: custome icons for project view
  - fix: convert to encoding setting in compiler set option not correctly handled
  - change: rename "compile log" panel to "tools output"
  - fix: debug panel can't be correctly show/hide
  - enhancement: redesign tools output's context menu, add "clear" menu item
  - enhancement: tools -> git  in the options dialog
  - enhancement: auto detect git in PATH
  - enhancement: git - create repository
  - enhancement: git - add files
  - enhancement: git - commit
  - enhancement: git - restore
  - enhancement: git - branch / switch
  - enhancement: git - merge
  - fix: compiler set index not correctly saved, when remove compiler sets in options dialog
  - enhancement: when create a repository in a project, auto add it's files to the repository
  - enhancement: when add files to project, auto add it to git (if the project has a git repository)
  - enhancement: when save a file, and it's under files view's current folder,  auto add it to git (if it has a git repository)
  - enhancement: new file icons for high contrast icon set
  - fix: left and bottom panel size not correct when DPI changed
  - fix: icons in files view not changed, when icon set is changed
  

Red Panda C++ Version 0.14.2
  - enhancement: file system view mode for project
  - enhancement: remove / rename / create new folder in the files view
  - fix: crash when there are catch blocks in the upper most scope
  - fix: can't read project templates when path has non-ascii chars
  - fix: huge build size for c++ files

Red Panda C++ Version 0.14.1
  - enhancement: custom theme
  - fix: failed to show function tip, when there are parameters having '[' and ']'
  - enhancement: display localized theme name in the option dialog
  - enhancement: show custom theme folder in options dialog -> enviroment -> folders
  - enhancement: display localzed icon set name in the option dialog
  - enhancement: new sky blue icon set, contributed by Alan-CRL
  - enhancement: show caret at once, when edition finished
  - enhancement: new header dialog for project
  - enhancement: new contrast icon set, contributed by Alan-CRL
  - enhancement: new contrast theme, contributed by Alan-CRL
  - enhancement: theme now have default icon set
  - fix: wrong icons for file associations
  - fix: editor's font size set by ctrl+mouse wheel will be reset by open the option dialog
  - fix: actions not correctly disabled when compile
  - fix: can't differentiate disabled and enabled buttons, when using contrast icon set
  - fix: when running problem cases, the output textbox might be wrongly cleared.
  - fix: typo error in the parser
  - fix: typing after symbols like 'std::string' shouldn't show code completion suggestions

Red Panda C++ Version 0.14.0
  - enhancement: custom icon set ( in the configuration folder)
  - enhancement: show custom icon set folder in options -> enviroment -> folders 
  - enhancement: new class ( to project) wizard
  - enhancement: greatly speed up code completion 
  - fix: code folding calcuation not correct when some codes are folded and editing after them
  - enhancement: code completion ui redesigned
  - fix: mainwindow action's short cut doesn't work,  if the action is not in menu or toolbar
  - fix: when run all cases for a problem, processing of output is slow

Red Panda C++ Version 0.13.4
  - fix: when copy comments, don't auto indent
  - enhancement: auto add a new line when press enter between '/*' and '*/'
  - fix: code completion popup won't show  members of 'this'
  - fix: can't show private & protected members of 'this'
  - fix: function name like 'A::B' is not correctly parsed
  - fix: static members are not correct shown after Classname + '::'
  - enhancement: show parameter tips for class constructors
  - enhancement: when there are tips showing, don't show mouse tips
  - enhancement: setting non-ascii font for editors
  - enhancement: correct handle windows dpi change event
  - enhancement: code completion find words with char in the middle

Red Panda C++ Version 0.13.3
  - enhancement: restore editor position after rename symbol
  - enhancement: restore editor position after reformat code
  - fix: If project's compiler set is not the same with the default compiler set, parser for the project doesn't use the project's compiler set
  - fix: If project's compiler set is not the same with the default compiler set, auto openned project's file will use wrong compiler set to do syntax check.
  - change: symbols that exactly match are sorted to the front in the code suggestion popup list
  - fix: symbols defind locally should be sorted to the front in the code suggestion popup list
  - fix: when show function tips, can't correctly calcuate the current position in the function param list
  - fix: app will become very slow when processing very long lines.
  - enhancement: If console pauser doesn't exist, warn and stop running programs.
  - fix: app crash when ctrl+click on a #include statement that point to a directory instead of header file.
  - fix: ctrl+click on the enum value will jump to the wrong line in it's definition file
  - fix: line info in the mouse tip of statement not correct
  - fix: editor crash when no highlighter is assigned (the editing file is a not c/cpp source file);
  - fix: ')' not correctly skip in the editor when no highlighter is assigned (the editing file is a not c/cpp source file);
  - fix: Undo in the editor will lose line indents when no highlighter is assigned (the editing file is a not c/cpp source file);
  - enhancement: highlighter for GLSL (OpenGL Shading Language)
  - add a new template for raylib shader apps
  - fix: project files' charset settings doesn't work correctly
  - enhancement: add exec charset option to compiler set settings
  - enhancement: delete to word begin /delete to word end
  - fix: when open a file, all blank lines's indents are removed.
  - fix: indent lines displayed at wrong position, when there are folded lines
  - fix: if editor's active line color is disabled, caret's position may not be correct redrawn
  - fix: insert code snippets will crash, if current compiler set's include dir list is not empty and lib dir list is empty
  - fix: search around option can't be disabled
  - enhancement: show a confirm dialog when search/replace around
  - enhancement: auto zoom ui when screen's zoom factor changed (windows)
  - enhancement: parser not called when open a file, if option "clean parser symbols when hidden" is turned on.
  
Red Panda C++ Version 0.13.2
  - fix: "delete and exit" button in the environtment / folder option page doesn't work correctly 
  - fix: crash when closing the options dialog under Ubuntu 20.04 LTS ( no memory leak now)
  - enhancement: can add non-code file in templates
  - enhancement: if there's no selection when copy/cut, select currect line by default
  - enhancement: support ligatures in fonts like fira code ( disabled by default, can be turned on in options dialog's editor font page)
  - enhancement: add "minimum id length required to show code completion" to the options dialog's editor code completion page
  - enhancement: modify values in the memory view while debugging
  - enhancement: auto update watch, local and memory view after expression evaluated
  - enhancement: auto update watch, local and memory view after memory modified
  - enhancement: modify values in the watch view by double click
  - fix: crash when refactor symbol and cursor is at the end of the identifier
  - fix: refactor symbol doesn't work for 1-length identifiers
  - enhancement: redirect stdio to a file while debugging ( must use gdb server mode to debug)
  - fix: parser can't correctly handle variable definitions that don't have spaces like 'int*x';
  - fix: parser can't correctly handle function parameters like 'int *x' 
  - fix: caret dispears when at '\t' under Windows  7
  - enhancement: ctrl+up/down scrolls in the editor
  - enhancement: add "wrap around" option to find/replace
  - fix: project's icon setting is not correctly saved
  - fix: project's type setting won't be saved
  - fix: If project's compiler set is not the same with the default compiler set, auto openned project's file will use wrong compiler set to do syntax check.
  - fix: open a project file through "File"->"Open" will not correctly connect it with the project internally
  - fix: wrong project program directory parameter is sent to the debugger
  - enhancement: better behavior of mouse tips
  - fix: in linux, projects no need of winres to be built

Red Panda C++ Version 0.13.1
 - enhancement: suppoort localization info in project templates
 - change: template / project files use utf-8 encoding instead of ANSI
 - fix: .rc file shouldn't be syntax checked
 - enhancement: auto save/restore size of the new project dialog
 - fix: new project dialog's tab bar should fill all empty spaces
 - enhancement: add raylib to autolinks
 - enhancement: distribute raylib with integrated gcc

Red Panda C++ Version 0.12.7
 - change: make current build system follow FHS specifications
 - fix: crash when close settings dialog in Ubuntu 20.04 (but we'll leak memory now...)
 - enhancement: add raylib.h to autolink
 - fix: shouldn't generate default autolink settings in linux
 - fix: shouldn't auto add /bin/gcc to compiler sets
 - fix: if a dir duplicates in PATH, don't add it to compiler sets repeatedly
 - enhancement: add "--sanitize=address" to compile option in the Debug compiler set in Linux 
 - enhancement: auto sort files in the project view

Red Panda C++ Version 0.12.6
 - fix: heartbeat for gdb server async command shouldn't disable actions
 - fix: problem cases doesn't use svg icons
 - fix: problem's title info not updated after running cases 
 - enhancement: open the corresponding source file from problem's context menu
 - fix: debugger's "continue" button not correctly disabled
 - change: use libicu instead of ConvertUTF.c under Linux
 - change depends to qt 5.12 instead of 5.15

Red Panda C++ Version 0.12.5
 - fix: compile error in linux
 - fix: can't receive gdb async output for commands
 - fix: can't reformat code
 - enhancement: add option for setting astyle path
 - fix: wrong file wildcard (*.*) in linux
 - fix: open terminal in linux
 - fix: wrong executable filename for source files in linux
 - enhancement: console pauser for linux 
 - enhancement: redirect input to program in linux
 - enhancement: detach pausing console window
 - rename to Red Pand C++

Version 0.12.4 For Dev-C++ 7 Beta
 - change: add copyright infos to each source file
 - fix: watch and local infos not updated when changing current frame in the call stack panel
 - enhancement: pause the debugging program (The debugger should work under gdb server mode, which is turned off by default in windows)

Version 0.12.3 For Dev-C++ 7 Beta
 - enhancement: basic linux compatibility
 - enhancement: debug with gdb server

Version 0.12.2 For Dev-C++ 7 Beta
 - enhancement: auto find compiler sets in the PATH 
 - change: path for iconsets
 - enhancement: select icon sets in options dialog ( but we  have only 1 icon set now...)

Version 0.12.1 For Dev-C++ 7 Beta
 - fix: error when drag&drop in editors

Version 0.12.0 For Dev-C++ 7 Beta
 - enhancement: enable run/debug/compile when console program finished but pausing.

Version 0.11.5 For Dev-C++ 7 Beta
 - fix: step into instruction and step over instruction not correctly disabled when cpu dialog is created
 - enhancement: icons in all dialogs auto change size with fonts 
 - enhancement: save/restore sizes of CPU dialog and settings dialog

Version 0.11.4 For Dev-C++ 7 Beta
 - fix: compiler set's custom link parameters  not used when compiling
 - fix: code completion doesn't work when input inside () or []
 - fix: auto indent processing error when input '{' in the middle of if statement
 - fix: left and right gutter offset settings not  correctly saved
 - fix: symbol completion for '<>' in the preprocessor line not work
 - enhancement: new svg icons set
 - enhancement: the size of icons in the main window zooms with font size

Version 0.11.3 For Dev-C++ 7 Beta
 - fix: use pixel size for fonts, to fit different dpi in multiple displays
 - enhancement: use the new expression parser to parse info for tips
 - enhancement: better highlight processing for preprocess directives 
 - enhancement: use the new expression parser to implement rename symbol
 - fix: rename symbol shouldn't remove empty lines

Version 0.11.2 For Dev-C++ 7 Beta
 - fix: button "run all problem cases" not disabled when compiling or debugging
 - enhancement: set font for problem case input/output textedits
 - enhancement: when run program with problem cases, updates output immediately (note: stdout of the program running with problem cases is fully buffered,
 so we need to fflush after each time output to stdout, or use setbuf(stdout,NULL) to turn the buffer off)
 - fix: current line of the disassembly in the cpu window not correctly setted
 - enhancement: add "step into one machine instruction" and "step over one machine instruction" in the cpu window
 - fix: can't correctly set TDM-GCC compiler
 - fix: auto add 32-bit compiler sets for TDM64-GCC

Version 0.11.1 For Dev-C++ 7 Beta
 - enhancement: Problem's test case shouldn't accept rich text inputs
 - enhancement: recalc layout info for code editors when dpi changed

Version 0.11.0 For Dev-C++ 7 Beta
 - enhancement: redesign the expression parser for code completion
 - fix: "make as default language" option in the project wizard doesn't work
 - fix: "ake as default language" option in the project wizard doesn't work
 - fix: typo errors in settings dialog
 - enhancement: console pauser clears STDIN buffer before show "press any key to continue..."
 - fix: path in macros should use system's path separator
 - fix: custom tools doesn't work
 - enhancement: add a demo for custom tool 

Version 0.10.4 For Dev-C++ 7 Beta
 - fix: can't correctly undo/redo indent 
 - fix: can't correctly undo/redo unindent
 - change: press tab when there are selections will do indent
 - change: press shift+tab when there are selections will do unindent
 - enhancement: press home will switch between begin of line and the position of fisrt non-space char
 - enhancement: press end will switch between end of line and the position of last non-space char 
 - enhancement: use "Microsoft Yahei" as the default UI font whe running in Simplified Chinese Windows

Version 0.10.3 For Dev-C++ 7 Beta
 - enhancement: treat files ended with ".C" or ".CPP"  as C++ files
 - enhancement: add option "ignore spaces when validating problem cases" to the "Executor"/"Problem Set" option tab.

Version 0.10.2 For Dev-C++ 7 Beta
 - fix: select by mouse can't correctly set mouse's column position
 - fix: dragging out of the editor and back will cause error
 - fix: dragging text from lines in the front to lines back will cause error
 - fix: dragging text onto itself should do nothing
 - fix：license info in the about dialog should be readonly
 - enhancement: change project name in the project view

Version 0.10.1 For Dev-C++ 7 Beta
 - fix: can't correctly expand watch expression that has spaces in it
 - fix: can't correctly display stl containers in watch
 - fix: the last line in the debug console is not correctly displayed
 - enhancement: scroll while dragging text in the editor
 - fix: dragging out of the editor shouldn't reset the caret back 

Version 0.10.0 For Dev-C++ 7 Beta
 - enhancement: use gdb/mi interface to  communicate with gdb debug session
 - enhancement: better display of watch vars
 - fix: project's modified flag not cleared after saved

Version 0.9.4 For Dev-C++ 7 Beta
 - fix: code formatter's "indent type" option not correctly saved

Version 0.9.3 For Dev-C++ 7 Beta
 - fix: the count in the title of issues view isn't correct
 - fix: columns calculation not correct when paint lines containing chinese characters
 - fix: restore caret position after reformat code
 - enhancement: ask user to rebuild project, when run/debug the project and it has been modified
 - fix: correct set the enabled state of "delete line"/"insert line"/"delete word"/"delete to BOL"/"delete to EOL" menu items
 - fix: undo "delete word"/"delete to BOL"/"delete to EOL" correct reset caret position

Version 0.9.2 For Dev-C++ 7 Beta
 - fix: gutter of the disassembly code control in the cpu info dialog is grayed
 - fix: problem set & problem views not correctly hidden when disabled in the executor / problem set options 
 - fix: executor / problem set options not correctly saved
 - fix: option "Move caret to the first non-space char in the current line when press HOME key" dosen't work fine.
 - fix: ctrl+left can't correctly move to the beginning of the last word
 - enhancement: add "delete line"/"duplicate line"/"delete word"/"delete to EOL"/"delete to BOL" in the edit menu
 - fix: crash when run "Project" / "Clean Make files"
 - fix: when make project and del non-existing files, shouldn't show error messages

Version 0.9.1 For Dev-C++ 7 Beta
 - enhancement: code completion suggestion for "__func__" variable
 - fix: ide failed to start, if there are errors in the compiler set settings
 - fix: numpad's enter key doesn't work
 - enhancement: code completion suggestion for phrase after long/short/signed/unsigned
 - enhancement: save/load default projects folder
 - enhancement: add editor general options "highlight current word" and "highlight matching braces"

Version 0.9.0 For Dev-C++ 7 Beta
 - fix: control keys in the numpad doesn't work in the editor
 - fix: project layout infos are wrongly saved to registry 
 - fix: project layout infos are not correctly saved/loaded

Version 0.8.11 For Dev-C++ 7 Beta
 - fix: text color for cpu info dialog not correctly setted

Version 0.8.10 For Dev-C++ 7 Beta
 - fix: Shouldn't update auto link settings, if the header name to be modified is unchanged
 - fix: add unit to project not correctly set new unit file's encoding
 - fix: correctly set encoding for the new added project unit file
 - fix: if there's a project openned, new file should ask user if he want to add the new file to the project
 - fix: when adding a file openned in the editor to the project, properties of it are not correctly setted.
 - enhancement: when remove a file from the project, also ask if user want to remove it from disk
 - fix: double click a project's .dev file in the Files panel should load the project

Version 0.8.9 For Dev-C++ 7 Beta
 - fix: text color of labels in statusbar not correctly updated when change theme

Version 0.8.8 For Dev-C++ 7 Beta
 - enhancement: drag & drop text in the editor
 - enhancement: auto calcuate caret line size basing on font size
 - enhancement: shift+mouse wheel to scroll horizontally 
 - fix: greatly reduces paste time 
 - fix: auto indent shouldn't use preprocessor's indent to calculate 
 - fix: option "don't add leading zeros to line numbers" not work
 - fix: "collapse all" and "uncollapse all" doesn't work

Version 0.8.7 For Dev-C++ 7 Beta
 - enhancement: auto indent line to column 1 when enter '#' at beginning of line
 - fix: when enter '{' or '}' at beginning of line, auto indent will remove all contents of the line
 - fix: auto indent should be turned off when reformat code
 - fix: auto indent should be turned off when replace in code 

Version 0.8.6 For Dev-C++ 7 Beta
 - enhancement: greatly reduces memory usage for symbol parsing ( memory needed for bits/stdc++.h reduced from 150m+ to 80m+)
 - fix: currect compiler set not correctly updated when switch between normal file and project file
 - fix: editor auto save settings not saved and applied
 - fix: only auto save files that has new modifications 
 - fix: correctly auto save files with it's own name

Version 0.8.5 For Dev-C++ 7 Beta
 - enhancement: use lighter color to draw menu seperators
 - enhancement: differentiate selected and unselected tab bars

Version 0.8.4 For Dev-C++ 7 Beta
 - enhancement: auto save/load the default open folder in the configuration file
 - fix: shouldn't auto add '()' when char succeeding the completed function name is '('
 - fix: can't show code completion popup if symbol is proceed with an operator '~' ( and it's not a destructor)
 - fix: can't show code completion popup when define MACRO
 - fix: can't debug files with chinese characters in the path

Version 0.8.3 For Dev-C++ 7 Beta
 - enhancement: View menu
 - enhancement: hide/show statusbar
 - enhancement: hide/show left/bottom tool window bars
 - enhancement: hide/show individual left/bottom tool window

Version 0.8.2 For Dev-C++ 7 Beta
 - fix: highlighter can't correctly find the end of ANSI C-style Comments
 - enhancement: add default color scheme to themes. Change theme option will change color scheme too.
 - fix: when changing options in the option dialog's color scheme panle, color of the demo editor won't be not correctly updated
 - enhancement: auto clear parsed symbols when the editor is hidden ( to reduce memory usage of un-active editors)
 - fix: when inputing in the editor, correctly set the position of the input method panel
 - fix: correctly display watch & local variable names when debugging

Version 0.8.1 For Dev-C++ 7 Beta
 - fix: ConsolePaurser.exe only exits when press ENTER
 - fix: input/output/expected textedit in the problem view shouldn't autowrap lines
 - fix: Red Panda C++ will freeze when receiving contents from Competitve Companion in chrome/edge
 - enhancement: when problem from competitive companion received, activate RedPanda C++ if it's minimized.
 - enhancement: when problem from competitive companion received, show the problem and problem set views.
 - enhancement: set problem's answer source file 
 - enhancement: open the problem's answer source file in editor
 - fix: if the proceeding line ends with ':' in comments, current line should not indent
 - enhancement: right click the problem set name label to rename it
 - change: memory view and locals view use debug console's font settings
 - fix: one line 'while' statement dosen't correctly indents
 - fix: line start with  '{' that follow an un-ended 'if'/'for' statement is not correctly un-indented
 - fix: multi-line comments indents calculation
 - fix: Installer should install the app in "program files", not "program files (x86)"
 - fix: symbol completion for '/*' not work
 - fix: javadoc-style docstring indents calculation
 - fix: indents calculation for the line succeeding "*/"

Version 0.8 For Dev-C++ 7 Beta
 - fix: find in the current file is not correcly saved in the search history
 - fix: hit info not correctly displayed in the search result view
 - fix: If find in files found no hits, search result view will not be shown.
 - fix: wront indents when paste one line content
 - fix: Results of "find symbol usage" in project not correctly set in the search result view
 - change: turn on gcc compiler's "-pipe" option by default, to use pipe instead of temp files in compiliation (and make the life of SSD longer)
 - fix: correctly save input histories for the find combo box in the Find dialog
 - fix: can't correctly test if it's not running in green mode

Version 0.7.8
 - enhancement: In problem view's output control, indicates which line is different with the expected
 - fix: current input/expected not correctly applied when save/run problem cases
 - fix: colors of the syntax issues view are not correctly set using the current color sheme
 - change: The error color of color scheme "vs code" 
 - add: "C Reference" in the help menu
 - fix: Custom editor colors shouldn't be tested for high contrast with the default background color
 - fix: Custom color settings not correctly displayed in the options widget
 - enhancement: add hit counts in the search result view
 - fix: editor actions' state not correctly updated after close editors.
 - fix: When replace in the editor, "Yes to All" and "No" button doesn't work correctly.
 - fix: crash when editing non-c/c++ files
 - enhancement: set the alpha value of scheme colors
 - enhancement: can use symbols' own foreground color to draw selection or the current line
 - enhancement: can use different colors to highlight the current word and the selections
 - enhancement: can set editor's default background / foreground color. They must be setted to make the custom color schemes correctly.
 - enhancement: can set the color for the current line's number in the gutter
 - all predefined color schemes updated. 
 - enhancement: check syntax/parse symbols when modifed and cursor's line changed.
 - enhancement: edit problem properties
 - enhancement: show problem description in the problem name lable's tooltip

Version 0.7.7
 - enhancement: Problem Set 
 - enhancement: Competitive Companion Support
 - change: "save" action will be enabled no matter contents in the current editor is modified or not
 - fix: focus not correctly set when the current editor is closed
 - fix: can't parse old c-style enum variable definition like "enum Test test;"
 - fix: remove the file change monitor if it's remove from the disk
 - fix: don't test if a file is writable before save to it (because qt can't do that test reliably).
 - fix: when search in project, files opened for search shouldn't be parsed for symbols.
 - fix: when search in project, the search history is not correctly updated.

Version 0.7.6
 - change: don't auto insert a new line when input an enter between '(' and ')' or between '[' and ']' (indent instead)
 - enhancement: the line containing '}' will use the indents of the matching '{' line, instead of just unindent one level
 - enhancement: the line containing 'public:' / 'private:' / 'protected:' / 'case *:' will use of indents of the surrounding '{' line, instead of just unindent one level
 - enhancement: correctly handle auto indents for multi-level embedding complex statements like 'for(...) if (...) printf();
 - change: Don't use 'pause' in the console pauser, in case of privilege problems.
 - enhancement: correctly handle auto indents for statement span many lines;
 - enhancment: only use colors have good contrasts with the background in the class browser and code completion suggestion window
 - fix: bottom and left panel properties not correctly saved when hiding the main window
 - fix: When debugging, if value of the variable pointed by the mouse cursor is too long, tooltip will fill the whole screen.

Version 0.7.5
 - enhancement: more accurate auto indent calculation
 - change: remove "add indent" option in the editor general options widget ( It's merged with "auto indent" option)
 - enhancement: auto insert a new line when input an enter between '(' and ')' or between '[' and ']'
 - fix: correctly updates cursor position when pasting from clipboard
 - enhancement: auto unindent when input protected: public: private: case *:
 - enhancement: can use PageDown / PageUp / Home / End to scroll in the auto completion popup

Version 0.7.4
 - fix: when debug a project, and have breakpoints that not in opened editors, dev-cpp will crash
 - fix: when a file is parsing in background, exit dev-cpp will crash
 - fix: "tab to spaces" option in the editor general options widget doesn't work
 - fix: when remove all breakpoints in the debug breakpoint view,  debug tags in the opened editors are not correctly updated.
 - change: when start debuging, show local view instead of the debug console.
 - update bundled compiler to msys2 mingw-w64 gcc 11.2 and gdb 10.2
 - update bundled xege to the lastest git build

Version 0.7.3
 - enhancement: icons in project view
 - fix: sometimes option widget will show confirm dialog even not changed
 - enhancement: only editor area will receive file drop events
 - enhancement: change project file's folder by drag and drop in the project view
 - enhancement: open project file by drag it to the editor area
 - fix: the "add bookmark" menu item is not correctly disabled on a bookmarked line
 - enhancement: "use utf8 by default" in editor's misc setting
 - fix: syntax issues not correctly cleared when the file was saved as another name.
 - enhancement: when running a program, redirect a data file to its stdin
 - fix: can't correctly handle '&&' and '||' in the #if directive (and correctly parse windows.h header file)
 - fix: crash when create an empty project
 - fix: syntax issues' filepath info not correct when build projects 
 - fix: compiler autolinks options widget don't show autolink infos
 - fix: autolink parameters are repeated when compile single files
 - enhancement: prompt for filename when create new project unit file
 - fix: options not correctly set when change compiler set in the project settings
 - change: reset compiler settings when change the project compiler set
 - enhancement: use project's compiler set type info to find a nearest system compiler set, when the project compiler set is not valid.
 - fix: toolbar's compiler set info not correctly updated when change it in the project settings dialog.

Version 0.7.2
 - fix: rainbow parenthesis stop functioning when change editor's general options
 - fix: issue count not correctly displayed when syntax check/compile finished
 - fix: function declaration's parameters not correctly parsed, if it have a definition which have different parameter names
 - fix: file path seperator used in the app is not unified, and cause errors somtimes.


Version 0.7.1
 - fix: can't add bookmark at a breakpoint line
 - fix: app name in the title bar not translated
 - use new app icon

Version 0.7.0
 - fix: Backspace still works in readonly mode
 - fix: save as file dialog's operation mode is not correct
 - enhancement: fill indents in the editor (Turned off by default)
 - enhancement: new file template
 - fix: when an editor is created, its caret will be displayed even it doesn't have focus
 - enhancement: set mouse wheel scroll speed in the editor general option tab ( 3 lines by default)
 - fix: don't highlight '#' with spaces preceeding it as error
 - fix: correctly handle integer with 'L' suffix in #if directives ( so <thread> can be correctly parsed )
 - enhancement: bookmark view
 - enhancement: autosave/load bookmarks
 - enhancement: autosave/load breakpoints 
 - enhancement: autosave/load watches
 - implement: files view
 - fix: app's title not update when editor closed

Version 0.6.8
 - enhancement: add link to cppreference in the help menu
 - fix: add mutex lock to prevent editor crash in rare conditions
 - fix: In the create project dialog, the browser button doesn't work
 - enhancement: use QStyle to implement the dark style, and better control of the style's look and feel 
 - enhancement: add link to EGE website, if locale is zh_CN

Version 0.6.7
 - fix: messages send to the gdb process's standard error are not received
 - adjust: the max value of the debug console's vertical scrollbar.
 - fix: shfit+click not correctly set selection's end
 - fix: ctrl+home/end not correctly set cursor to start/end of the editor
 - enhancement: click the encoding info in the statusbar will show encoding menu

Version 0.6.6
 - fix: crash when create new file
 - implement: two editor view

Version 0.6.5
 - implement: export as rtf / export as html
 - fix: the contents copied/exported are not correctly syntax colored
 - fix: stop execution if the source file is not compiled and user choose not to compile it
 - fix: not correctly parse gdb's output
 - fix: path not correctly setted for the debugger process
 - fix: indent line not correctly drawed
 - enhancement: use rainbox color to draw indent guide lines
 - implement: highlight matching brackets

Version 0.6.4
 - fix: code completion popup not show after '->' inputted
 - fix: font styles in the color scheme settings not in effect
 - fix: editor's font style shouldn't affect gutter's font style
 - change: enable copy as HTML by default
 - fix: unneeded empty lines when copy as HTML

Version 0.6.3
 - fix: should use c++ syntax to check ".h" files
 - fix: can't copy contents in a readonly editor
 - fix: project's file not correctly syntaxed when open in editor
 - libturtle update: add fill() / setBackgroundColor() /setBackgroundImage() functions
 - fix: code fold calculation not correct, when editing code
 - fix: can't correctly find definition of the symbols in namespace
    
Version 0.6.2 
 - fix: The Enter key in the numpad doesn't work
 - fix: The compiled executable not fully write to the disk before run it
 - fix: settings object not correctly released when exit
 - fix: shouldn't check syntax when save modifications before compiling
 - fix: shouldn't scroll to the end of the last line when update compile logs
 - fix: can't debug project

Version 0.6.1
 - fix: editor deadlock

Version 0.6.0
 - fix: old data not displayed when editing code snippets
 - fix: shift-tab for unindent not work
 - fix: can't save code snippets modifications
 - fix: errors in code snippet processing
 - change: auto open a new editor at start
 - enhancement: todo view
 - add: about dialog
 - implement: correctly recognize clang (msys2 build)
 - enhancement: don't add encoding options when using clang to compile (clang only support utf-8)
 - enhancement: find occurence in project
 - implement: rename symbol in file
 - enhancement: replace in files
 - enhancement: rename symbol in project (using search symbol occurence and replace in files)
 - fix: search in files
 - implement: register file associations
 - implement: when startup , open file provided by command line options
 - implement: open files pasted by clipboard
 - fix: code fold parsing not correct
 - enhancement: support #include_next (and clang libc++)
 - fix:  hide popup windows when the editor is closed
 - enhancement: show pinyin when input chinese characters
 - fix: add mutex lock to prevent rare conditions when editor is modifying and the content is read
 - fix: makefile generated for static / dynamic library projects not right
 - fix: editors disappeared when close/close all
 - implement: config shortcuts
 - implement: handle windows logout message
 - fix: editor's inproject property not correctly setted (and may cause devcpp to crash when close project)
 - implement: print
 - implement: tools configuration
 - implement: default settings for code formatter
 - implement: remove all custom settings

Version 0.5.0
 - enhancement: support C++ using type alias;
 - fix: when press shift, completion popu window will hide
 - enhancement: options in debugger setting widget, to skip system/project/custom header&project files when step into
 - fix: icon not correctly displayed for global variables in the class browser 
 - enhancement: more charset selection in the edit menu
 - fix: can't correctly get system default encoding name when save file
 - fix: Tokenizer can't correctly handle array parameters
 - fix: debug actions enabled states not correct updated when processing debug mouse tooltips
 - enhancement: redesign charset selection in the project options dialog's file widget
 - fix: can't correctly load last open files / project with non-asii characters in path
 - fix: can't coorectly load last open project
 - fix: can't coorectly show code completion for array elements
 - enhancement: show caret when show code/header completions
 - fix: correctly display pointer info in watch console
 - implement: search in project
 - enhancement: view memory when debugging
 - implement: symbol usage count
 - implement: user code snippet / template
 - implement: auto generate javadoc-style docstring for functions
 - enhancement: use up/down key to navigate function parameter tooltip
 - enhancement: press esc to close function parameter tooltip
 - enhancement: code suggestion for unicode identifiers
 - implement: context menu for debug console
 - fix: errors in debug console
 - fix: speed up the parsing process of debugger
 - ehancement: check if debugger path contains non-ascii characters (this will prevent it from work

Version 0.2.1
 - fix: crash when load last opens

Version 0.2
 - fix : header file completion stop work when input '.'
 - change: continue to run / debug if there are compiling warnings (but no errors)
 - enhancement: auto load last open files at start
 - enhancement: class browser syntax colors and icons
 - enhancement: function tips
 - enhancement: project support
 - enhancement: paint color editor use system palette's disabled group color
 - fix: add watch not work when there's no editor openned;
 - enhancement: rainbow parenthesis
 - enhancement: run executable with parameters
 - add: widget for function tips
 - enhancement: options for editor tooltips
 - fix: editor folder process error
