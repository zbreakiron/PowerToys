﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using Community.PowerToys.Run.Plugin.VSCodeWorkspaces.Properties;
using Community.PowerToys.Run.Plugin.VSCodeWorkspaces.RemoteMachinesHelper;
using Community.PowerToys.Run.Plugin.VSCodeWorkspaces.VSCodeHelper;
using Community.PowerToys.Run.Plugin.VSCodeWorkspaces.WorkspacesHelper;
using Wox.Plugin;

namespace Community.PowerToys.Run.Plugin.VSCodeWorkspaces
{
    public class Main : IPlugin, IPluginI18n
    {
        public PluginInitContext _context { get; private set; }

        public string Name => GetTranslatedPluginTitle();

        public string Description => GetTranslatedPluginDescription();

        public Main()
        {
            VSCodeInstances.LoadVSCodeInstances();
        }

        public readonly VSCodeWorkspacesApi _workspacesApi = new VSCodeWorkspacesApi();

        public readonly VSCodeRemoteMachinesApi _machinesApi = new VSCodeRemoteMachinesApi();

        public List<Result> Query(Query query)
        {
            var results = new List<Result>();

            if (query != null)
            {
                // Search opened workspaces
                _workspacesApi.Workspaces.ForEach(a =>
                {
                    var title = $"{a.FolderName}";

                    var typeWorkspace = a.WorkspaceTypeToString();
                    if (a.TypeWorkspace == TypeWorkspace.Codespaces)
                    {
                        title += $" - {typeWorkspace}";
                    }
                    else if (a.TypeWorkspace != TypeWorkspace.Local)
                    {
                        title += $" - {(a.ExtraInfo != null ? $"{a.ExtraInfo} ({typeWorkspace})" : typeWorkspace)}";
                    }

                    var tooltip = new ToolTipData(title, $"{Resources.Workspace}{(a.TypeWorkspace != TypeWorkspace.Local ? $" {Resources.In} {typeWorkspace}" : "")}: {SystemPath.RealPath(a.RelativePath)}");

                    results.Add(new Result
                    {
                        Title = title,
                        SubTitle = $"{Resources.Workspace}{(a.TypeWorkspace != TypeWorkspace.Local ? $" {Resources.In} {typeWorkspace}" : "")}: {SystemPath.RealPath(a.RelativePath)}",
                        Icon = a.VSCodeInstance.WorkspaceIcon,
                        ToolTipData = tooltip,
                        Action = c =>
                        {
                            bool hide;
                            try
                            {
                                var process = new ProcessStartInfo
                                {
                                    FileName = a.VSCodeInstance.ExecutablePath,
                                    UseShellExecute = true,
                                    Arguments = $"--folder-uri {a.Path}",
                                    WindowStyle = ProcessWindowStyle.Hidden
                                };
                                Process.Start(process);

                                hide = true;
                            }
                            catch (Win32Exception)
                            {
                                var name = $"Plugin: {_context.CurrentPluginMetadata.Name}";
                                var msg = "Can't Open this file";
                                _context.API.ShowMsg(name, msg, string.Empty);
                                hide = false;
                            }
                            return hide;
                        },
                        ContextData = a,
                    });
                });


                // Search opened remote machines
                _machinesApi.Machines.ForEach(a =>
                {
                    var title = $"{a.Host}";

                    if (a.User != null && a.User != string.Empty && a.HostName != null && a.HostName != string.Empty)
                    {
                        title += $" [{a.User}@{a.HostName}]";
                    }

                    var tooltip = new ToolTipData(title, Resources.SSHRemoteMachine);

                    results.Add(new Result
                    {
                        Title = title,
                        SubTitle = Resources.SSHRemoteMachine,
                        Icon = a.VSCodeInstance.RemoteIcon,
                        ToolTipData = tooltip,
                        Action = c =>
                        {
                            bool hide;
                            try
                            {
                                var process = new ProcessStartInfo()
                                {
                                    FileName = a.VSCodeInstance.ExecutablePath,
                                    UseShellExecute = true,
                                    Arguments = $"--new-window --enable-proposed-api ms-vscode-remote.remote-ssh --remote ssh-remote+{((char)34) + a.Host + ((char)34)}",
                                    WindowStyle = ProcessWindowStyle.Hidden,
                                };
                                Process.Start(process);

                                hide = true;
                            }
                            catch (Win32Exception)
                            {
                                var name = $"Plugin: {_context.CurrentPluginMetadata.Name}";
                                var msg = "Can't Open this file";
                                _context.API.ShowMsg(name, msg, string.Empty);
                                hide = false;
                            }
                            return hide;
                        },
                        ContextData = a,
                    });
                });
            }

            if (query.ActionKeyword == string.Empty || (query.ActionKeyword != string.Empty && query.Search != string.Empty))
            {
                results = results.Where(a => a.Title.ToLowerInvariant().Contains(query.Search.ToLowerInvariant())).ToList();
            }

            results.ForEach(x =>
            {
                if (x.Score == 0)
                {
                    x.Score = 100;
                }

                //intersect the title with the query
                var intersection = Convert.ToInt32(x.Title.ToLowerInvariant().Intersect(query.Search.ToLowerInvariant()).Count() * query.Search.Count());
                var differenceWithQuery = Convert.ToInt32((x.Title.Count() - intersection) * query.Search.Count() * 0.7);
                x.Score = x.Score - differenceWithQuery + intersection;

                //if is a remote machine give it 12 extra points
                if (x.ContextData is VSCodeRemoteMachine)
                {
                    x.Score = Convert.ToInt32(x.Score + intersection * 2);
                }
            });

            results = results.OrderByDescending(x => x.Score).ToList();
            if (query.Search == string.Empty || query.Search.Replace(" ", "") == string.Empty)
            {
                results = results.OrderBy(x => x.Title).ToList();
            }

            return results;
        }

        public void Init(PluginInitContext context)
        {
            _context = context;
        }

        public string GetTranslatedPluginTitle()
        {
            return Resources.PluginTitle;
        }

        public string GetTranslatedPluginDescription()
        {
            return Resources.PluginDescription;
        }
    }
}
