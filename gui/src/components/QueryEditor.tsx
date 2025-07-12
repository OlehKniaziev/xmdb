import Editor, { useMonaco } from "@monaco-editor/react";
import { useEffect } from "react";
import BottomPanel from "./BottomPanel";
import { Panel, PanelGroup, PanelResizeHandle } from "react-resizable-panels";

function QueryEditor() {
  const monaco = useMonaco();
  useEffect(() => {
    if (monaco) {
      monaco.editor.defineTheme("warm-orange", {
        base: "vs",
        inherit: true,
        rules: [
          { token: "", background: "FBFEFB" },
          { token: "comment", foreground: "a59f85" },
          { token: "keyword", foreground: "f78c6c" },
          { token: "string", foreground: "ecc48d" },
          { token: "number", foreground: "ffcb6b" },
          { token: "variable", foreground: "ffd580" },
        ],
        colors: {
          "editor.background": "#FBFEFB",
          "editor.lineHighlightBackground": "#FBFEFB",
          "editorCursor.foreground": "#ffcb6b",
          "editorLineNumber.foreground": "#EFE5DC",
          "editorLineNumber.activeForeground": "#D0B8AC",
          "editor.selectionBackground": "#EFE5DC",
          "editor.inactiveSelectionBackground": "#EFE5DC",
        },
      });
      monaco.editor.setTheme("warm-orange");
    }
  }, [monaco]);
  return (
    <>
      <PanelGroup direction="vertical">
        <Panel>
          <Editor
            defaultLanguage="sql"
            defaultValue="-- start writing your code here"
            theme="warm-orange"
            className="editor-style"
          />
        </Panel>
        <PanelResizeHandle />
        <Panel minSize={6} defaultSize={20}>
          <BottomPanel />
        </Panel>
      </PanelGroup>
    </>
  );
}

export default QueryEditor;
