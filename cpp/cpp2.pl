#!/usr/bin/perl
use strict;
use warnings;
use File::Basename;
use Cwd 'abs_path';

# 全局变量
my %macros;
my @if_stack;
my $skip_level = 0;
my $current_file;
my %included_files;
my $in_comment = 0;
my %expansion_depth;  # 跟踪宏展开深度，防止无限递归

# 主程序
sub main {
    my ($input_file, $output_file) = parse_args();
    
    open my $out_fh, '>', $output_file or die "无法打开输出文件 $output_file: $!";
    $current_file = $input_file;
    
    process_file($input_file, $out_fh);
    
    close $out_fh;
}

# 解析命令行参数
sub parse_args {
    die "用法: $0 <input> -o <output>" unless @ARGV == 3 && $ARGV[1] eq '-o';
    return ($ARGV[0], $ARGV[2]);
}

# 处理文件内容
sub process_file {
    my ($file, $out_fh) = @_;
    
    my $abs_path = abs_path($file);
    return if $included_files{$abs_path}++;
    
    open my $in_fh, '<', $file or die "无法打开输入文件 $file: $!";
    
    while (my $line = <$in_fh>) {
        chomp $line;
        
        # 处理行继续符
        while ($line =~ s/\\\s*$//) {
            my $next_line = <$in_fh>;
            last unless defined $next_line;
            chomp $next_line;
            $line .= $next_line;
        }
        
        # 处理注释
        $line = process_comments($line);
        next unless defined $line && $line ne '';
        
        # 处理预处理指令
        if ($line =~ /^\s*#\s*(\w+)/) {
            my $directive = $1;
            
            if ($directive eq 'define') {
                handle_define($line, $out_fh) unless $skip_level;
            }
            elsif ($directive eq 'undef') {
                handle_undef($line) unless $skip_level;
            }
            elsif ($directive eq 'include') {
                handle_include($line, $out_fh) unless $skip_level;
            }
            elsif ($directive eq 'ifdef') {
                handle_ifdef($line);
            }
            elsif ($directive eq 'ifndef') {
                handle_ifndef($line);
            }
            elsif ($directive eq 'else') {
                handle_else();
            }
            elsif ($directive eq 'endif') {
                handle_endif();
            }
            next;
        }
        
        # 如果不是预处理指令且不在跳过模式下，处理宏替换并输出
        unless ($skip_level) {
            $line = expand_macros_recursive($line);
            print $out_fh "$line\n";
        }
    }
    
    close $in_fh;
}

# 处理注释
sub process_comments {
    my ($line) = @_;
    my $output = '';
    
    if ($in_comment) {
        if ($line =~ m|\*/|) {
            $line =~ s|^.*?\*/||;
            $in_comment = 0;
        } else {
            return undef;
        }
    }
    
    $line =~ s|//.*||;
    
    while ($line =~ m|/\*|) {
        if ($line =~ m|/\*.*?\*/|) {
            $line =~ s|/\*.*?\*/||g;
        } else {
            $line =~ s|/\*.*||;
            $in_comment = 1;
        }
    }
    
    return $line =~ /\S/ ? $line : undef;
}

# 递归展开宏
sub expand_macros_recursive {
    my ($line) = @_;
    my $last_line;
    my $depth = 0;
    
    # 循环直到宏展开不再改变或达到最大深度
    do {
        $last_line = $line;
        $line = expand_macros($line);
        
        # 防止无限递归
        if ($depth++ > 50) {
            warn "宏展开可能进入无限递归: $line";
            last;
        }
    } while ($line ne $last_line);
    
    return $line;
}

# 处理 #define 指令
sub handle_define {
    my ($line, $out_fh) = @_;
    
    # 匹配三种情况：
    # 1. #define NAME
    # 2. #define NAME value
    # 3. #define NAME(params) value
    if ($line =~ /^\s*#\s*define\s+(\w+)\s*(?:\((.*?)\))?(?:\s+((?:\\\n|.)*))?$/) {
        my ($name, $args, $value) = ($1, $2, $3);
        
        # 处理行继续符
        if (defined $value) {
            $value =~ s/\\\n//g;
        }
        
        # 处理带参数的宏
        if (defined $args) {
            $args = [split /\s*,\s*/, $args];
            $macros{$name} = { type => 'function', args => $args, value => $value // '' };
        }
        else {
            # 处理空定义宏
            $macros{$name} = { type => 'object', value => defined $value ? $value : '' };
        }
    }
}

# 处理 #undef 指令
sub handle_undef {
    my ($line) = @_;
    $line =~ /^\s*#\s*undef\s+(\w+)/;
    delete $macros{$1};
}

# 处理 #include 指令
sub handle_include {
    my ($line, $out_fh) = @_;
    
    if ($line =~ /^\s*#\s*include\s*["<]([^">]+)[">]/) {
        my $include_file = $1;
        my $dir = dirname($current_file);
        my $path;
        
        for my $search_path ($dir, '/usr/include', '/usr/local/include') {
            $path = "$search_path/$include_file";
            if (-e $path) {
                $current_file = $path;
                process_file($path, $out_fh);
                $current_file = "$dir/x";
                return;
            }
        }
        
        warn "无法找到包含文件: $include_file";
    }
}

# 处理 #ifdef 指令
sub handle_ifdef {
    my ($line) = @_;
    $line =~ /^\s*#\s*ifdef\s+(\w+)/;
    my $condition = exists $macros{$1};
    
    push @if_stack, { level => $skip_level, active => $condition };
    $skip_level++ unless $condition;
}

# 处理 #ifndef 指令
sub handle_ifndef {
    my ($line) = @_;
    $line =~ /^\s*#\s*ifndef\s+(\w+)/;
    my $condition = !exists $macros{$1};
    
    push @if_stack, { level => $skip_level, active => $condition };
    $skip_level++ unless $condition;
}

# 处理 #else 指令
sub handle_else {
    my $if = pop @if_stack;
    $skip_level = $if->{level} + ($if->{active} ? 1 : 0);
    push @if_stack, { level => $if->{level}, active => 0 };
}

# 处理 #endif 指令
sub handle_endif {
    my $if = pop @if_stack;
    $skip_level = $if->{level};
}

# 基础宏展开
sub expand_macros {
    my ($line) = @_;
    
    # 持续展开直到没有宏为止
    my $changed;
    do {
        $changed = 0;
        
        # 使用更强大的括号匹配正则表达式
        while ($line =~ /(\w+)                     # 宏名称
                        \(                        # 左括号
                        (                         # 开始捕获参数
                            (?:                   # 非捕获组
                                [^()]++           # 非括号字符
                                |                 # 或
                                \( (?-1) \)       # 平衡括号
                            )*                    # 重复
                        )                         # 结束捕获参数
                        \)                        # 右括号
                        /gx) {
            my ($macro_name, $args_str) = ($1, $2);
            next unless exists $macros{$macro_name} && $macros{$macro_name}{type} eq 'function';
            
            my $macro = $macros{$macro_name};
            my @args = split_balanced_args($args_str);
            
            if (@args == @{$macro->{args}}) {
                my $value = $macro->{value};
                for my $i (0..$#args) {
                    my $arg_name = $macro->{args}[$i];
                    # 递归展开参数
                    $args[$i] = expand_macros($args[$i]);
                    $value =~ s/\b$arg_name\b/$args[$i]/g;
                }
                $line =~ s/\Q$macro_name($args_str)\E/$value/;
                $changed = 1;
            }
        }
        
        # 处理对象宏
        foreach my $macro_name (keys %macros) {
            next if $macros{$macro_name}{type} eq 'function';
            
            my $value = $macros{$macro_name}{value};
            if ($line =~ /\b$macro_name\b/) {
                $line =~ s/\b$macro_name\b/$value/g;
                $changed = 1;
            }
        }
        
    } while ($changed);
    
    return $line;
}

# 更强大的参数分割函数，处理任意嵌套括号
sub split_balanced_args {
    my ($args_str) = @_;
    my @args;
    my $depth = 0;
    my $current = '';
    
    foreach my $char (split //, $args_str) {
        if ($char eq ',' && $depth == 0) {
            push @args, $current;
            $current = '';
            next;
        }
        
        $current .= $char;
        $depth++ if $char eq '(';
        $depth-- if $char eq ')';
        
        # 确保不会出现不匹配的括号
        die "Unbalanced parentheses in macro arguments" if $depth < 0;
    }
    
    die "Unbalanced parentheses in macro arguments" if $depth != 0;
    push @args, $current if $current ne '';
    return map { s/^\s+|\s+$//g; $_ } @args;
}

main();
