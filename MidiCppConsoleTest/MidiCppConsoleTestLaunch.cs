// Copyright (c) Kodi Studios 2021.
// Licensed under the MIT license.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Diagnostics;
using System.IO;

namespace MidiCppConsoleTest
{
    [TestClass]
    public class MidiCppConsoleTestLaunch
    {
        [TestMethod]
        public void DefaultLaunch()
        {
            // Launch OneNoteMidi.exe
            using (Process oneNoteMidiProcess = Process.Start(GetMidiCppConsoleFilePath()))
            {
                oneNoteMidiProcess.WaitForExit();

                Assert.AreEqual(0, oneNoteMidiProcess.ExitCode);
            }
        }

        string GetMidiCppConsoleFilePath()
        {
            // Sample TestContext.TestRunDirectory:
            // C:\Users\naris\Source\Repos\one-note-midi\one-note-midi\OneNoteMidi\TestResults\Deploy_naris 2021-03-28 18_00_44
            // It's weird that TestResults are inside of OneNoteMidi folder, not OneNoteMidiTest...

            string oneNoteMidiProjectPath = Path.GetDirectoryName(
                Path.GetDirectoryName(TestContext.TestRunDirectory));

            string oneNoteMidiFilePath = Path.Combine(oneNoteMidiProjectPath, Flavor, "MidiCppConsole.exe");

            return oneNoteMidiFilePath;
        }

        #region Context Infrastructure
        private TestContext instance;

        public TestContext TestContext
        {
            set { instance = value; }
            get { return instance; }
        }
        #endregion

#if DEBUG
        const string Flavor = "Debug";
#else
        const string Flavor = "Release";
#endif
    }
}
