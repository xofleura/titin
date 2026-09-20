# hi guys, yes it was me (lb) who made this
# this is the only ruby file in the whole thing so i just left this here for yall

output = ARGV.shift
sources = ARGV

File.open(output, "w") do |file|
  file.puts '#include "amino.h"'
  file.puts

  sources.each_with_index do |source, index|
    text = File.read(source)
    lines = text.lines

    if lines.empty?
      abort "empty Amino file: #{source}"
    end

    parts = lines[0].strip.split

    unless parts.length == 2 && parts[0] == "command"
      abort "invalid command declaration: #{source}"
    end

    name = parts[1]

    escaped = text
      .gsub(/\\/) { '\\\\' }
      .gsub(/"/) { '\\"' }
      .gsub(/\n/) { '\\n' }

    file.puts "static const char amino_source_#{index}[] = \"#{escaped}\";"
    file.puts "static const amino_command amino_command_#{index} __attribute__((section(\"amino_commands\"), used)) = { \"#{name}\", amino_source_#{index} };"
    file.puts
  end
end
