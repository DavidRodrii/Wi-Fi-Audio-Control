﻿using System;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;

namespace ImportEQ
{
    class Program
    {
        static void Main(string[] args)
        {
            var path = $@"{Directory.GetCurrentDirectory()}\..\..\..\..\..\Server";
            var files = Directory.GetFiles(path, "*.hz");
            var output = new StringBuilder($"byte bands[{files.Length}][21][5][4] = \r\n{{");

            // Do some dirty string replacement hacks for each file
            foreach (var file in files)
            {
                var input = File.ReadAllText(file);

                // Replace empty lines with: },{
                var a = input.Replace("\r\n\r\n", "\r\n\t\t},{\r\n");
                //File.WriteAllText(Path.Combine(path, "a.h"), a);

                // Replace last , with: },
                var b = a.Replace(",\t\r\n", "},\r\n ");
                //File.WriteAllText(Path.Combine(path, "b.h"), b);

                // Add tabs at beginning of line
                var c = b.Replace(" 0x", "\t\t\t{ 0x");
                //File.WriteAllText(Path.Combine(path, "c.h"), c);

                // Prepend first index of each array with tabs
                var d = c.Replace("\r\n0x", "\r\n\t\t\t{ 0x");
                //File.WriteAllText(Path.Combine(path, "d.h"), d);

                // Prepend whole string and cut of last part
                var e = "\t{\r\n\t\t{\r\n\t\t\t{ " + d.Substring(0, d.Length - 4) + "\r\n\t}";
                //File.WriteAllText(Path.Combine(path, "e.h"), e);


                output.AppendLine();
                //output.AppendLine("{");
                output.AppendLine(e);
                //output.AppendLine("},");
                output.AppendLine(",");
            }

            // Replace last , by ;
            var final = output.ToString().Substring(0, output.ToString().Length - 3) + "};";

            File.WriteAllText(Path.Combine(path, "eq.h"), final);
        }
    }
}