﻿using System;
using System.Text.RegularExpressions;

namespace Community.PowerToys.Run.Plugin.VSCodeWorkspaces.WorkspacesHelper
{
    public class ParseVSCodeUri
    {
        private static readonly Regex LocalWorkspace = new Regex("^file:///(.+)$", RegexOptions.Compiled);

        private static readonly Regex RemoteSSHWorkspace = new Regex(@"^vscode-remote://ssh-remote\+(.+?(?=\/))(.+)$", RegexOptions.Compiled);

        private static readonly Regex RemoteWSLWorkspace = new Regex(@"^vscode-remote://wsl\+(.+?(?=\/))(.+)$", RegexOptions.Compiled);

        private static readonly Regex CodespacesWorkspace = new Regex(@"^vscode-remote://vsonline\+(.+?(?=\/))(.+)$", RegexOptions.Compiled);

        public static (TypeWorkspace? TypeWorkspace, String MachineName, String Path) GetTypeWorkspace(string uri)
        {
            if (LocalWorkspace.IsMatch(uri))
            {
                var match = LocalWorkspace.Match(uri);

                if (match.Groups.Count > 1)
                {
                    return (TypeWorkspace.Local, null, match.Groups[1].Value);
                }
            }
            else if (RemoteSSHWorkspace.IsMatch(uri))
            {
                var match = RemoteSSHWorkspace.Match(uri);

                if (match.Groups.Count > 1)
                {
                    return (TypeWorkspace.RemoteSSH, match.Groups[1].Value, match.Groups[2].Value);
                }
            }
            else if (RemoteWSLWorkspace.IsMatch(uri))
            {
                var match = RemoteWSLWorkspace.Match(uri);

                if (match.Groups.Count > 1)
                {
                    return (TypeWorkspace.RemoteWSL, match.Groups[1].Value, match.Groups[2].Value);
                }
            }
            else if (CodespacesWorkspace.IsMatch(uri))
            {
                var match = CodespacesWorkspace.Match(uri);

                if (match.Groups.Count > 1)
                {
                    return (TypeWorkspace.Codespaces, String.Empty, match.Groups[2].Value);
                }
            }

            return (null, null, null);
        }
    }
}
