import React, { useState } from 'react';
import { 
  FileCode, 
  Copy, 
  Check, 
  Download, 
  Folder, 
  FileText, 
  CheckCircle2, 
  ExternalLink 
} from 'lucide-react';
import { FIRMWARE_FILES } from '../data/firmwareFiles';
import { FirmwareFile } from '../types';

export const CodeExplorer: React.FC = () => {
  const [selectedFile, setSelectedFile] = useState<FirmwareFile>(FIRMWARE_FILES[0]);
  const [copied, setCopied] = useState(false);

  const handleCopy = () => {
    navigator.clipboard.writeText(selectedFile.content);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleDownload = () => {
    const blob = new Blob([selectedFile.content], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = selectedFile.filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  };

  return (
    <div className="space-y-6">
      {/* Top Banner */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-sm flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
        <div>
          <h2 className="text-base font-bold text-white flex items-center gap-2">
            <FileCode className="w-5 h-5 text-emerald-400" />
            مرورگر فایل‌های سورس فریم‌ور STM32CubeIDE
          </h2>
          <p className="text-xs text-slate-400 mt-0.5">
            کدهای تولید شده برای میکروکنترلر STM32F407VGT6 شامل درایورهای ADC با DMA، سوئیچینگ مالتی‌پلکسر، فیلترها و فایل‌های بیلد.
          </p>
        </div>

        {/* Total stats */}
        <div className="flex gap-2 text-xs font-mono">
          <span className="px-3 py-1.5 rounded-md bg-slate-800 border border-slate-700 text-slate-300">
            {FIRMWARE_FILES.length} ماژول سورس و هدر
          </span>
          <span className="px-3 py-1.5 rounded-md bg-emerald-900/30 border border-emerald-500/30 text-emerald-300">
            STM32CubeIDE Ready
          </span>
        </div>
      </div>

      {/* Main Split Layout: File Tree Sidebar (4 cols) & Code Viewer (8 cols) */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Sidebar: File List */}
        <div className="lg:col-span-4 space-y-3">
          <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-sm">
            <div className="p-3 bg-slate-800/80 border-b border-slate-700/60 font-mono text-xs font-bold text-slate-300 flex items-center gap-2">
              <Folder className="w-4 h-4 text-amber-400" />
              <span>ساختار شاخه firmware/</span>
            </div>

            <div className="divide-y divide-slate-800/60 p-1.5">
              {FIRMWARE_FILES.map(file => {
                const isSelected = selectedFile.path === file.path;
                return (
                  <button
                    key={file.path}
                    id={`file-btn-${file.filename.replace('.', '_')}`}
                    onClick={() => setSelectedFile(file)}
                    className={`w-full text-left p-2.5 rounded-lg transition-colors flex items-start gap-2.5 ${
                      isSelected
                        ? 'bg-emerald-500/10 text-emerald-300 border border-emerald-500/30'
                        : 'hover:bg-slate-800/60 text-slate-300'
                    }`}
                  >
                    <FileCode className={`w-4 h-4 mt-0.5 shrink-0 ${isSelected ? 'text-emerald-400' : 'text-slate-500'}`} />
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center justify-between">
                        <span className="font-mono text-xs font-bold truncate block">{file.filename}</span>
                        <span className="text-[10px] font-mono px-1.5 py-0.2 rounded bg-slate-800 text-slate-400 border border-slate-700">
                          {file.category}
                        </span>
                      </div>
                      <span className="text-[11px] text-slate-400 block truncate mt-0.5 font-sans">
                        {file.description}
                      </span>
                    </div>
                  </button>
                );
              })}
            </div>
          </div>
        </div>

        {/* Main Code Viewer */}
        <div className="lg:col-span-8 bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-sm flex flex-col h-[600px]">
          {/* Header */}
          <div className="bg-slate-800/80 px-4 py-2.5 border-b border-slate-700/60 flex items-center justify-between font-mono text-xs">
            <div className="flex items-center gap-2 text-slate-200">
              <span className="text-emerald-400 font-bold">{selectedFile.path}</span>
            </div>

            {/* Action buttons: Copy & Download */}
            <div className="flex items-center gap-2">
              <button
                id="btn-copy-code"
                onClick={handleCopy}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-slate-700 hover:bg-slate-600 text-slate-200 transition-colors"
              >
                {copied ? (
                  <>
                    <Check className="w-3.5 h-3.5 text-emerald-400" />
                    <span className="text-emerald-400">کپی شد!</span>
                  </>
                ) : (
                  <>
                    <Copy className="w-3.5 h-3.5" />
                    <span>کپی کد</span>
                  </>
                )}
              </button>

              <button
                id="btn-download-file"
                onClick={handleDownload}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-emerald-600 hover:bg-emerald-500 text-white font-medium transition-colors"
              >
                <Download className="w-3.5 h-3.5" />
                <span>دانلود فایل</span>
              </button>
            </div>
          </div>

          {/* Description bar */}
          <div className="bg-slate-950 px-4 py-2 border-b border-slate-800 text-xs text-slate-400 flex items-center justify-between">
            <span>{selectedFile.description}</span>
            <span className="font-mono text-[11px] text-slate-500">
              {selectedFile.content.split('\n').length} خط کد
            </span>
          </div>

          {/* Code Viewer Body */}
          <div className="flex-1 overflow-auto p-4 bg-slate-950 font-mono text-xs text-slate-200 leading-relaxed scrollbar-thin scrollbar-thumb-slate-800">
            <pre className="text-slate-300">
              <code>{selectedFile.content}</code>
            </pre>
          </div>
        </div>
      </div>
    </div>
  );
};
