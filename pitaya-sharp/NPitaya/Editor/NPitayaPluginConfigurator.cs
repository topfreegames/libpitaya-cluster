// Excluded from Unity 6 because the .meta layout shipped with NPitaya >= 1.0.11
// works as authored there. On Unity 2021/2022 the same .meta files do not filter
// the per-architecture native libs correctly: linux-x86_64/linux-armv8 both claim
// the Linux64 slot, macos-x86_64/macos-arm64 both claim OSXUniversal, Unity sees
// a duplicate-filename conflict and silently excludes them all, and the build
// then crashes at runtime with DllNotFoundException for libpitaya_cpp.
//
// This is a build-output post-process: after Unity finishes the build, we stamp
// the correct native library into the player's plugins directory, replacing any
// wrong-arch variant Unity may have copied and creating the file if Unity
// dropped them all. Source assets and .meta files are never touched, so working
// trees stay clean. On Unity 6 the new layout works as authored, so the whole
// file is excluded from compilation there.
#if !UNITY_6000_0_OR_NEWER

using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEngine;
using PackageInfo = UnityEditor.PackageManager.PackageInfo;

namespace NPitaya.Editor
{
    internal sealed class NPitayaPluginConfigurator : IPostprocessBuildWithReport
    {
        private const string PackageId = "com.wildlifestudios.npitaya";
        private const string AssetsFallback = "Assets/NPitaya";

        public int callbackOrder => 1000;

        public void OnPostprocessBuild(BuildReport report)
        {
            var target = report.summary.platform;
            var outputPath = report.summary.outputPath;
            if (string.IsNullOrEmpty(outputPath))
            {
                return;
            }

            var packageRoot = ResolvePackageRoot();
            if (packageRoot == null)
            {
                return;
            }

            var relativeSource = PickSourceLib(target);
            if (relativeSource == null)
            {
                return;
            }

            var sourceAbs = Path.Combine(packageRoot, relativeSource);
            if (!File.Exists(sourceAbs))
            {
                Debug.LogWarning("[NPitaya] Source native missing, skipping: " + sourceAbs);
                return;
            }

            var fileName = Path.GetFileName(sourceAbs);
            foreach (var dest in LocateDestinations(target, outputPath, fileName))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(dest));
                File.Copy(sourceAbs, dest, overwrite: true);
                Debug.Log("[NPitaya] Stamped native plugin: " + dest);
            }
        }

        // If Unity already produced one or more libpitaya_cpp.* files in the build
        // (which may be the correct or the wrong arch), we overwrite each in place.
        // If Unity dropped them all (the conflict case), we place a single file at
        // the target's conventional plugins location.
        private static IEnumerable<string> LocateDestinations(
            BuildTarget target, string outputPath, string fileName)
        {
            var root = target == BuildTarget.StandaloneOSX
                ? outputPath
                : Path.GetDirectoryName(outputPath);

            if (!string.IsNullOrEmpty(root) && Directory.Exists(root))
            {
                var existing = Directory.GetFiles(root, fileName, SearchOption.AllDirectories);
                if (existing.Length > 0)
                {
                    return existing;
                }
            }

            var fallback = DefaultDestination(target, outputPath, fileName);
            return fallback == null ? System.Array.Empty<string>() : new[] { fallback };
        }

        private static string DefaultDestination(
            BuildTarget target, string outputPath, string fileName)
        {
            if (target == BuildTarget.StandaloneOSX)
            {
                return Path.Combine(outputPath, "Contents", "PlugIns", fileName);
            }

            var dir = Path.GetDirectoryName(outputPath);
            if (string.IsNullOrEmpty(dir) || !Directory.Exists(dir))
            {
                return null;
            }

            var dataDirs = Directory.GetDirectories(dir, "*_Data", SearchOption.TopDirectoryOnly);
            var dataDir = dataDirs.Length > 0
                ? dataDirs[0]
                : Path.Combine(dir, Path.GetFileNameWithoutExtension(outputPath) + "_Data");

            var pluginsDir = target == BuildTarget.StandaloneWindows64
                ? Path.Combine(dataDir, "Plugins", "x86_64")
                : Path.Combine(dataDir, "Plugins");

            return Path.Combine(pluginsDir, fileName);
        }

        private static string PickSourceLib(BuildTarget target)
        {
            switch (target)
            {
                case BuildTarget.StandaloneLinux64:
                    // Unity < 6000 has no Linux ARM64 standalone target; always x86_64.
                    return "Runtime/Plugins/runtimes/linux-x86_64/libpitaya_cpp.so";
                case BuildTarget.StandaloneWindows64:
                    return "Runtime/Plugins/runtimes/windows-x86_64/libpitaya_cpp.dll";
                case BuildTarget.StandaloneOSX:
                    // OSXUniversal is a single slot pre-Unity 6 and we ship per-arch
                    // dylibs, so we pick by editor host. Cross-arch macOS builds need
                    // Unity 6 or a fat dylib (not shipped here).
                    return IsEditorHostArm64()
                        ? "Runtime/Plugins/runtimes/macos-arm64/libpitaya_cpp.dylib"
                        : "Runtime/Plugins/runtimes/macos-x86_64/libpitaya_cpp.dylib";
                default:
                    return null;
            }
        }

        private static bool IsEditorHostArm64()
        {
            return System.Runtime.InteropServices.RuntimeInformation.OSArchitecture
                == System.Runtime.InteropServices.Architecture.Arm64;
        }

        // Returns the package's absolute filesystem root (UPM packages resolve via
        // PackageInfo, vendored Assets/NPitaya copies fall back to project-relative).
        private static string ResolvePackageRoot()
        {
            var info = PackageInfo.FindForAssetPath("Packages/" + PackageId + "/package.json");
            if (info != null && !string.IsNullOrEmpty(info.resolvedPath))
            {
                return info.resolvedPath;
            }

            return AssetDatabase.IsValidFolder(AssetsFallback)
                ? Path.GetFullPath(AssetsFallback)
                : null;
        }
    }
}

#endif
