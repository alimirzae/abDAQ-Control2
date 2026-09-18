import React, { useState } from 'react';
import { BookOpen, FileText, Copy, Check } from 'lucide-react';
import { DOCS_DATA, DocItem } from '../data/docsContent';

export const DocReader: React.FC = () => {
  const [selectedDoc, setSelectedDoc] = useState<DocItem>(DOCS_DATA[0]);
  const [copied, setCopied] = useState(false);

  const handleCopy = () => {
    navigator.clipboard.writeText(selectedDoc.content);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="space-y-6">
      {/* Top Banner */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-sm flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
        <div>
          <h2 className="text-base font-bold text-white flex items-center gap-2">
            <BookOpen className="w-5 h-5 text-emerald-400" />
            مرکز مستندات و داکیومنت‌های مخزن
          </h2>
          <p className="text-xs text-slate-400 mt-0.5">
            فایل‌های تولید شده شامل راهنمای کامل سیم‌بندی، پروتکل‌ها، فایل‌های راهنما و پارامترهای حافظه پروژه.
          </p>
        </div>

        <div className="text-xs font-mono text-slate-400">
          مستندات دو زبانه (فارسی و انگلیسی)
        </div>
      </div>

      {/* Main Grid: Doc Tabs & Content */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Sidebar Nav (3 cols) */}
        <div className="lg:col-span-3 space-y-2">
          {DOCS_DATA.map(doc => {
            const isSelected = selectedDoc.id === doc.id;
            return (
              <button
                key={doc.id}
                id={`doc-tab-${doc.id}`}
                onClick={() => setSelectedDoc(doc)}
                className={`w-full text-left p-3 rounded-xl transition-all border flex items-center gap-3 ${
                  isSelected
                    ? 'bg-emerald-600/10 border-emerald-500/40 text-emerald-300'
                    : 'bg-slate-900 border-slate-800 text-slate-300 hover:bg-slate-850 hover:text-white'
                }`}
              >
                <FileText className={`w-4 h-4 shrink-0 ${isSelected ? 'text-emerald-400' : 'text-slate-500'}`} />
                <div className="min-w-0 flex-1">
                  <span className="text-xs font-bold block truncate">{doc.titleFa}</span>
                  <span className="text-[11px] text-slate-500 font-mono block truncate">{doc.filename}</span>
                </div>
              </button>
            );
          })}
        </div>

        {/* Content Viewer (9 cols) */}
        <div className="lg:col-span-9 bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-sm flex flex-col min-h-[500px]">
          {/* Header */}
          <div className="p-4 bg-slate-800/80 border-b border-slate-700/60 flex items-center justify-between">
            <div>
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <span>{selectedDoc.titleFa}</span>
                <span className="text-slate-500 font-normal">({selectedDoc.titleEn})</span>
              </h3>
              <span className="text-xs font-mono text-emerald-400">{selectedDoc.filename}</span>
            </div>

            <button
              id="btn-copy-doc"
              onClick={handleCopy}
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-slate-700 hover:bg-slate-600 text-xs font-mono text-slate-200 transition-colors"
            >
              {copied ? (
                <>
                  <Check className="w-3.5 h-3.5 text-emerald-400" />
                  <span className="text-emerald-400">کپی شد!</span>
                </>
              ) : (
                <>
                  <Copy className="w-3.5 h-3.5" />
                  <span>کپی محتوا</span>
                </>
              )}
            </button>
          </div>

          {/* Body */}
          <div className="p-6 overflow-y-auto font-sans text-sm text-slate-200 leading-relaxed space-y-4 max-h-[650px] scrollbar-thin scrollbar-thumb-slate-800">
            <div className="prose prose-invert max-w-none text-xs leading-relaxed whitespace-pre-wrap font-mono">
              {selectedDoc.content}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
