using System;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Diagnostics;

[assembly: AssemblyTitle("ScrollBench Portable")]
[assembly: AssemblyProduct("ScrollBench")]
[assembly: AssemblyVersion("1.0.0.0")]
[assembly: AssemblyFileVersion("1.0.0.0")]

class Program {
    static void Main() {
        string tempPath = Path.Combine(Path.GetTempPath(), "SB_" + Guid.NewGuid().ToString().Substring(0, 8));
        try {
            Directory.CreateDirectory(tempPath);
            
            // Extract Payload
            string zipPath = Path.Combine(tempPath, "payload.zip");
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("payload.zip")) {
                if(stream == null) { throw new Exception("Embedded payload not found."); }
                using (FileStream fileStream = new FileStream(zipPath, FileMode.Create)) {
                    stream.CopyTo(fileStream);
                }
            }
            
            // Unzip (Requires .NET 4.5+)
            // Using reflection to avoid direct reference issues if simpler environment
            // formatting: System.IO.Compression.ZipFile.ExtractToDirectory(zipPath, tempPath);
            System.IO.Compression.ZipFile.ExtractToDirectory(zipPath, tempPath);
            
            // Run executable
            string exePath = Path.Combine(tempPath, "ScrollBenchPortable.exe");
            if(File.Exists(exePath)) {
                ProcessStartInfo psi = new ProcessStartInfo(exePath);
                psi.WorkingDirectory = tempPath;
                Process p = Process.Start(psi);
                p.WaitForExit(); 
            } else {
                // Determine if we are in a nested folder (sometimes zips create a root folder)
                string[] exes = Directory.GetFiles(tempPath, "ScrollBenchPortable.exe", SearchOption.AllDirectories);
                if(exes.Length > 0) {
                     ProcessStartInfo psi = new ProcessStartInfo(exes[0]);
                     psi.WorkingDirectory = Path.GetDirectoryName(exes[0]);
                     Process.Start(psi).WaitForExit();
                }
            }
        } catch (Exception ex) {
            // Setup a simple error dialog using standard windows dll if possible, or just fail silently/log
            // Console.WriteLine("Error: " + ex.Message); 
        } finally {
            // Cleanup
            try { 
                if(Directory.Exists(tempPath)) Directory.Delete(tempPath, true); 
            } catch {}
        }
    }
}
