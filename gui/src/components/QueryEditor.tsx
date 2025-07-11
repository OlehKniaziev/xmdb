import Editor, { useMonaco } from "@monaco-editor/react";
import { useEffect } from "react";

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
    <Editor
      defaultLanguage="sql"
      defaultValue="-- start writing your code here"
      theme="warm-orange"
    />
  );
}

export default QueryEditor;
