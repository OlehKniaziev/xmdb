import Editor, { useMonaco } from "@monaco-editor/react";
import { useEffect } from "react";
import BottomPanel from "./BottomPanel";
import { Panel, PanelGroup, PanelResizeHandle } from "react-resizable-panels";
import { useMultiTabQueryStore } from "../data/global-states";
import "../styles/bars-style.css";

function QueryEditor() {
  const monaco = useMonaco();
  const { tabs, activeTabId, updateActiveTabQuery } = useMultiTabQueryStore();

  const activeTab = tabs.find(t => t.id === activeTabId);

  useEffect(() => {
    if (monaco) {
      monaco.editor.defineTheme("beige-theme", {
        base: "vs",
        inherit: true,

        rules: [
          { token: "", foreground: "210F04" },

          { token: "comment", foreground: "A3B18A", fontStyle: "italic" },

          { token: "keyword", foreground: "690500", fontStyle: "bold" },
          { token: "operator", foreground: "b60900" },

          { token: "string", foreground: "BB6B00" },
          { token: "string.sql", foreground: "BB6B00" },

          { token: "number", foreground: "934B00" },

          { token: "function", foreground: "6B3A4B" },
          { token: "type", foreground: "5C6B73" },
          { token: "class", foreground: "5C6B73" },

          { token: "variable", foreground: "210F04" },
          { token: "constant", foreground: "6B7A3A" },

          { token: "delimiter", foreground: "452103" },
        ],

        colors: {
          "editor.background": "#FBFEFB",
          "editor.foreground": "#210F04",

          "editorCursor.foreground": "#690500",

          "editor.lineHighlightBackground": "#EFE5DC",

          "editorLineNumber.foreground": "#D0B8AC",
          "editorLineNumber.activeForeground": "#452103",

          "editor.selectionBackground": "#D0B8AC",
          "editor.inactiveSelectionBackground": "#EFE5DC",

          "editorBracketHighlight.foreground1": "#934B00",
          "editorBracketHighlight.foreground2": "#6B3A4B",
          "editorBracketHighlight.foreground3": "#5C6B73",
          "editorBracketHighlight.foreground4": "#6B7A3A",

          "editorBracketMatch.background": "#EFE5DC",
          "editorBracketMatch.border": "#690500",

          "scrollbar.shadow": "#d1d8c4",
          "scrollbarSlider.background": "#c7d0b8cc",
          "scrollbarSlider.hoverBackground": "#b5c0a1dd",
          "scrollbarSlider.activeBackground": "#a3b18add",
          "editorScrollbarCorner.background": "#FBFEFB",
        },
      });
      monaco.editor.setTheme("beige-theme");

      monaco.languages.registerFoldingRangeProvider("sql", {
        provideFoldingRanges(model) {
          const ranges: { start: number; end: number; kind: typeof monaco.languages.FoldingRangeKind.Region }[] = [];
          const lineCount = model.getLineCount();
          for (let i = 1; i <= lineCount; i++) {
            if (/\bRGB\s*\(/.test(model.getLineContent(i))) {
              for (let j = i + 1; j <= lineCount; j++) {
                if (model.getLineContent(j).trim().startsWith(")")) {
                  if (j - 1 > i) {
                    ranges.push({
                      start: i,
                      end: j - 1,
                      kind: monaco.languages.FoldingRangeKind.Region,
                    });
                  }
                  break;
                }
              }
            }
          }
          return ranges;
        },
      });
    }
  }, [monaco]);

  if (!activeTab) {
      return <div className="p-4 text-center">No active query tab. Please create a new query.</div>;
  }

  return (
    <>
      <PanelGroup direction="vertical">
        <Panel>
          <Editor
            defaultLanguage="sql"
            defaultValue="-- start writing your code here"
            value={activeTab.query}
            theme="beige-theme"
            className="editor-style"
            onChange={(val) => updateActiveTabQuery(val || "")}
            options={{
              fontFamily: '"Fragment Mono", monospace',
              wordWrap: "on",
              folding: true,
              foldingStrategy: "auto",
              showFoldingControls: "always",
            }}
          />
        </Panel>
        <PanelResizeHandle />
        <Panel minSize={6} defaultSize={20}>
          <BottomPanel tabId={activeTab.id} />
        </Panel>
      </PanelGroup>
    </>
  );
}

export default QueryEditor;
