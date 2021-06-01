﻿// Copyright (c) Brice Lambson
// The Brice Lambson licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.  Code forked from Brice Lambson's https://github.com/bricelam/ImageResizer/

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.IO.Abstractions;
using System.Threading;
using System.Threading.Tasks;
using ImageResizer.Properties;

namespace ImageResizer.Models
{
    public class ResizeBatch
    {
        private readonly IFileSystem _fileSystem = new FileSystem();

        public string DestinationDirectory { get; set; }

        public ICollection<string> Files { get; } = new List<string>();

        public static ResizeBatch FromCommandLine(TextReader standardInput, string[] args)
        {
            var batch = new ResizeBatch();

            // NB: We read these from stdin since there are limits on the number of args you can have
            string file;
            if (standardInput != null)
            {
                while ((file = standardInput.ReadLine()) != null)
                {
                    batch.Files.Add(file);
                }
            }

            for (var i = 0; i < args?.Length; i++)
            {
                if (args[i] == "/d")
                {
                    batch.DestinationDirectory = args[++i];
                    continue;
                }

                batch.Files.Add(args[i]);
            }

            return batch;
        }

        public IEnumerable<ResizeError> Process(Action<int, double> reportProgress, CancellationToken cancellationToken)
        {
            double total = Files.Count;
            var completed = 0;
            var errors = new ConcurrentBag<ResizeError>();

            // TODO: If we ever switch to Windows.Graphics.Imaging, we can get a lot more throughput by using the async
            //       APIs and a custom SynchronizationContext
            Parallel.ForEach(
                Files,
                new ParallelOptions
                {
                    CancellationToken = cancellationToken,
                    MaxDegreeOfParallelism = Environment.ProcessorCount,
                },
                (file, state, i) =>
                {
                    try
                    {
                        Execute(file);
                    }
#pragma warning disable CA1031 // Do not catch general exception types
                    catch (Exception ex)
#pragma warning restore CA1031 // Do not catch general exception types
                    {
                        errors.Add(new ResizeError { File = _fileSystem.Path.GetFileName(file), Error = ex.Message });
                    }

                    Interlocked.Increment(ref completed);

                    reportProgress(completed, total);
                });

            return errors;
        }

        protected virtual void Execute(string file)
            => new ResizeOperation(file, DestinationDirectory, Settings.Default).Execute();
    }
}
