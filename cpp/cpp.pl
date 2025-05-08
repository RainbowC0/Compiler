#!/usr/bin/perl
use strict;
use warnings;
#use File::Slurp;
use 5.010;
use File::Basename;
use Cwd qw(abs_path);

# 参数解析
my ($input_file, $output_file);
while (@ARGV) {
    my $arg = shift @ARGV;
    if ($arg eq '-o') {
        $output_file = shift @ARGV or die "Missing output file after -o";
    } else {
        $input_file = $arg;
    }
}
die "Usage: $0 <source> -o <output>" unless $input_file && $output_file;

# 符号表存储宏定义
my %macros;
# 条件编译栈（记录当前是否处于活跃代码块）
my @if_stack = (1);  # 默认顶层为活跃状态

# 递归处理文件（支持 #include）
sub process_file {
    my ($file_path) = @_;
    my $dir = dirname(abs_path($file_path));
    my $content = `cat $file_path` or die "Cannot read $file_path";

    # 处理所有宏指令和替换
    $content =~ s{
        (?# 消除注释)
        //.*\n (?{$^R='';})
        |
        /\*([^*]|\*+[^*/])*(\*/\s*)? (?{$^R='';})
        |
        (?# 1. 匹配 #include "file" 或 #include <file> )
        ^\s*\#include \s+ (["<]) (?<file>[^">]+) [">] \s* $ 
        (?{
            my $include_file = $+{file};
            my $full_path;
            if ($1 eq '"') {
                $full_path = "$dir/$include_file";
            } else {  # <...> 搜索系统路径（简化版：当前目录）
                $full_path = $include_file;
            }
            process_file($full_path);  # 递归处理
        })

        |

        (?# 2. 匹配 #define )
        ^\s*\#\s*define\s+(?<name>\w+)(?:\s*\((?<args>.*?)\))?(?:[ \t\f]+(?<value>[^\n]*)\s*)?$ 
        (?{
            $macros{$+{name}} = $+{value} if $if_stack[-1];
        })

        |

        (?# 3. 匹配 #undef )
        ^\s*\#undef \s+ (?<name>\w+) \s* $ 
        (?{
            delete $macros{$+{name}} if $if_stack[-1];
        })

        |

        (?# 4. 匹配 #ifdef )
        ^\s*\#ifdef \s+ (?<name>\w+) \s* $ 
        (?{
            push @if_stack, ($if_stack[-1] && exists $macros{$+{name}});
        })

        |

        (?# 5. 匹配 #ifndef )
        ^\s*\#ifndef \s+ (?<name>\w+) \s* $ 
        (?{
            push @if_stack, ($if_stack[-1] && !exists $macros{$+{name}});
        })

        |

        (?# 6. 匹配 #else )
        ^\s*\#else \s* $ 
        (?{
            die "#else without #if" if @if_stack <= 1;
            $if_stack[-1] = $if_stack[-2] && !$if_stack[-1];
        })

        |

        (?# 7. 匹配 #endif )
        ^\s*\#endif \s* $ 
        (?{
            die "#endif without #if" if @if_stack <= 1;
            pop @if_stack;
        })

        |

        (?# 8. 匹配宏调用（仅在活跃块中替换） )
        (?<!\w) (?<call>\w+) (?!\w)
        (?{
            if ($if_stack[-1] && exists $macros{$+{call}}) {
                $^R = $macros{$+{call}};
            } else {
                $^R = $+{call};
            }
        })

        |

        (?# 其他内容：仅在活跃块中保留 )
        (?<other>[^\#]+)
        (?{
            $^R = $if_stack[-1] ? $+{other} : "";
        })
    }{$^R}xmge;

    return $content;
}

# 主处理流程
my $processed_content = process_file($input_file);

# 写入输出文件
print $processed_content;
print "Processed $input_file -> $output_file\n";
say "xasc";
