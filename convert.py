# https://jameshfisher.com/2016/11/30/c-multiline-literal/
import sys

fname = sys.argv[1]
with open(fname) as fr, open(fname.replace(".", "_") + '.h', 'w') as fw:
  lines = []
  for line in fr.readlines():
    line = line.strip()
    line = line.replace('"', '\\"')
    lines.append(line)
  _ = fw.write(f'char* {fname.replace(".", "_")} = "')
  _ = fw.write('\\n\\\n'.join(lines))
  _ = fw.write('";')

