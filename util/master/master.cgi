#!/usr/bin/perl

=info
install:
 cpan JSON JSON::XS
 touch list_full list log.log
 chmod a+rw list_full list log.log

freebsd:
 www/fcgiwrap www/nginx

rc.conf.local:
nginx_enable="YES"
fcgiwrap_enable="YES"
fcgiwrap_user="www"

nginx:

        location / {
            index  index.html;
            add_header Access-Control-Allow-Origin *;
        }
        location /announce {
            fastcgi_pass   unix:/var/run/fcgiwrap/fcgiwrap.sock;
            fastcgi_param  SCRIPT_FILENAME $document_root/master.cgi;
            include        fastcgi_params;
        }


apache .htaccess:
 AddHandler cgi-script .cgi
 DirectoryIndex index.html
 Options +ExecCGI +FollowSymLinks
 Order allow,deny
 <FilesMatch (\.(html?|cgi|fcgi|css|js|gif|png|jpe?g|ico)|(^)|\w+)$>
  Allow from all
 </FilesMatch>
 Deny from all
 <ifModule mod_headers.c>
     Header set Access-Control-Allow-Origin: *
 </ifModule>



=cut

use strict;
no strict qw(refs);
use warnings "NONFATAL" => "all";
no warnings qw(uninitialized);
no if $] >= 5.017011, warnings => 'experimental::smartmatch';
use utf8;
use Socket;
BEGIN {
    if ($Socket::VERSION ge '2.008') {
        eval qq{use Socket qw(getaddrinfo getnameinfo NI_NUMERICHOST NIx_NOSERV)}; # >5.16
    } else {
        eval qq{use Socket6 qw(getaddrinfo getnameinfo NI_NUMERICHOST NIx_NOSERV)}; # <5.16
    }
};
use Time::HiRes qw(time sleep);
use IO::Socket::IP;
use JSON;
use Net::Ping;
#use Data::Dumper;
our $root_path;
($ENV{'SCRIPT_FILENAME'} || $0) =~ m|^(.+)[/\\].+?$|;    #v0w
$root_path = $1 . '/' if $1;
$root_path =~ s|\\|/|g;

our %config = (
    #debug        => 1,
    list_full    => $root_path . 'list_full',
    list_pub     => $root_path . 'list',
    log          => $root_path . 'log.log',
    time_purge   => 86400 * 1,
    time_alive   => 650,
    source_check => 1,
    ping_timeout => 3,
    ping         => 1,
    mineping     => 1,
    pingable     => 1,
    trusted      => [qw( 176.9.122.10 )],       #masterserver self ip - if server on same ip with masterserver doesnt announced
    #blacklist => [], # [qw(2.3.4.5 4.5.6.7 8.9.0.1), '1.2.3.4', qr/^10\.20\.30\./, ], # list, or quoted, ips, or regex
);
do($root_path . 'config.pl');
our $ping = Net::Ping->new("udp", $config{ping_timeout});
$ping->hires();

sub get_params_one(@) {
    local %_ = %{ref $_[0] eq 'HASH' ? shift : {}};
    for (@_) {
        tr/+/ /, s/%([a-f\d]{2})/pack 'H*', $1/gei for my ($k, $v) = /^([^=]+=?)=(.+)$/ ? ($1, $2) : (/^([^=]*)=?$/, /^-/);
        $_{$k} = $v;
    }
    wantarray ? %_ : \%_;
}

sub get_params(;$$) {    #v7
    my ($string, $delim) = @_;
    $delim ||= '&';
    read(STDIN, local $_ = '', $ENV{'CONTENT_LENGTH'}) if !$string and $ENV{'CONTENT_LENGTH'};
    local %_ =
      $string
      ? get_params_one split $delim, $string
      : (get_params_one(@ARGV), map { get_params_one split $delim, $_ } split(/;\s*/, $ENV{'HTTP_COOKIE'}), $ENV{'QUERY_STRING'}, $_);
    wantarray ? %_ : \%_;
}

sub get_params_utf8(;$$) {
    local $_ = &get_params;
    utf8::decode $_ for %$_;
    wantarray ? %$_ : $_;
}

sub file_rewrite(;$@) {
    local $_ = shift;
    return unless open my $fh, '>', $_;
    print $fh @_;
}

sub printlog(;@) {
    #local $_ = shift;
    return unless open my $fh, '>>', $config{log};
    print $fh (join ' ', @_), "\n";
}

sub file_read ($) {
    open my $f, '<', $_[0] or return;
    local $/ = undef;
    my $ret = <$f>;
    close $f;
    return \$ret;
}

sub read_json {
    my $ret = {};
    eval { $ret = JSON->new->utf8->relaxed(1)->decode(${ref $_[0] ? $_[0] : file_read($_[0]) or \''} || '{}'); };    #'mc
    printlog "json error [$@] on [", ${ref $_[0] ? $_[0] : \$_[0]}, "]" if $@;
    $ret;
}

sub printu (@) {
    for (@_) {
        print($_), next unless utf8::is_utf8($_);
        my $s = $_;
        utf8::encode($s);
        print($s);
    }
}

sub float {
    return ($_[0] < 8 and $_[0] - int($_[0]))
      ? sprintf('%.' . ($_[0] < 1 ? 3 : ($_[0] < 3 ? 2 : 1)) . 'f', $_[0])
      : int($_[0]);
}

sub mineping ($$) {
    my ($addr, $port) = @_;
    printlog "mineping($addr, $port)" if $config{debug};
    my $data;
    my $time = time;
    eval {
        my $socket = IO::Socket::IP->new(
            'PeerAddr' => $addr,
            'PeerPort' => $port,
            'Proto'    => 'udp',
            'Timeout'  => $config{ping_timeout},
        );
        $socket->send("\x4f\x45\x74\x03\x00\x00\x00\x01");
        local $SIG{ALRM} = sub { die "alarm time out"; };
        alarm $config{ping_timeout};
        $socket->recv($data, POSIX::BUFSIZ) or die "recv: $!";
        alarm 0;
        1;    # return value from eval on normalcy
    } or return 0;
    return 0 unless length $data;
    $time = float(time - $time);
    printlog "recvd: ", length $data, " [$time]" if $config{debug};
    return $time;
}

sub request (;$) {
    my ($r) = @_;
    $r ||= \%ENV;
    my $param = get_params_utf8;
    my $after = sub {
        if ($param->{json}) {
            my $j = {};
            eval { $j = JSON->new->decode($param->{json}) || {} };
            $param->{$_} = $j->{$_} for keys %$j;
            delete $param->{json};
        }
        #printlog 'recv', Dumper $param;
        if (%$param) {
            s/^false$// for values %$param;
            $param->{ip} = $r->{REMOTE_ADDR};
            $param->{ip} =~ s/^::ffff://;
            for (@{$config{blacklist}}) {
                #printlog("blacklist", $param->{ip} ~~ $_) if $config{debug};
                return if $param->{ip} ~~ $_;
            }
            $param->{address} ||= $param->{ip};
            if ($config{source_check}) {
                (my $err, local @_) = getaddrinfo($param->{address});
                my $addrs = [ map{(getnameinfo($_->{addr}, NI_NUMERICHOST, NIx_NOSERV))[1]} @_];
                if (!($param->{ip} ~~ $addrs) and !($param->{ip} ~~ $config{trusted})) {
                    printlog("bad address (", @$addrs, ")[$param->{address}] ne [$param->{ip}] [$err]") if $config{debug};
                    return;
                }
            }
            $param->{port} ||= 30000;
            $param->{key} = "$param->{ip}:$param->{port}";
            $param->{off} = time if $param->{action} ~~ 'delete';
            if ($config{ping} and $param->{action} ne 'delete') {
                if ($config{mineping}) {
                    $param->{ping} = mineping($param->{ip}, $param->{port});
                } else {
                    $ping->port_number($param->{port});
                    $ping->service_check(0);
                    my ($pingret, $duration, $ip) = $ping->ping($param->{address});
                    if ($ip ne $param->{ip} and !($param->{ip} ~~ $config{trusted})) {
                        printlog "strange ping ip [$ip] != [$param->{ip}]" if $config{debug};
                        return if $config{source_check} and !($param->{ip} ~~ $config{trusted});
                    }
                    $param->{ping} = $duration if $pingret;
                    printlog " PING t=$config{ping_timeout}, $param->{address}:$param->{port} = ( $pingret, $duration, $ip )" if $config{debug};
                }
                return if !$param->{ping};
            }
            my $list = read_json($config{list_full}) || {};
            printlog "readed[$config{list_full}] list size=", scalar @{$list->{list}} if $config{debug};
            my $listk = {map { $_->{key} => $_ } @{$list->{list}}};
            my $old = $listk->{$param->{key}};
            $param->{time} = $old->{time} if $param->{off};
            $param->{time} ||= int time;
            $param->{start} = $param->{action} ~~ 'start' ? $param->{time} : $old->{start} || $param->{time};
            delete $param->{start} if $param->{off};
            $param->{clients} ||= scalar @{$param->{clients_list}} if ref $param->{clients_list} eq 'ARRAY';
            $param->{first} ||= $old->{first} || $old->{time} || $param->{time};
            $param->{clients_top} = $old->{clients_top} if $old->{clients_top} > $param->{clients};
            $param->{clients_top} ||= $param->{clients} || 0;
            # params reported once on start, must be same as src/serverlist.cpp:~221 if(server["action"] == "start") { ...
            for (qw(dedicated rollback liquid_finite mapgen mods privs)) {
                $param->{$_} ||= $old->{$_} if $old->{$_} and !($param->{action} ~~ 'start');
            }
            $param->{pop_n} = $old->{pop_n} + 1;
            $param->{pop_c} = $old->{pop_c} + $param->{clients};
            $param->{pop_v} = $param->{pop_c} / $param->{pop_n};
            delete $param->{action};
            $listk->{$param->{key}} = $param;
            #printlog 'write', Dumper $param if $config{debug};
            my $list_full = [grep { $_->{time} > time - $config{time_purge} } values %$listk];

            $list->{list} = [
                sort { $b->{clients} <=> $a->{clients} || $a->{start} <=> $b->{start} }
                  grep { $_->{time} > time - $config{time_alive} and !$_->{off} and (!$config{ping} or !$config{pingable} or $_->{ping}) }
                  @{$list_full}
            ];
            $list->{total} = {clients => 0, servers => 0};
            for (@{$list->{list}}) {
                $list->{total}{clients} += $_->{clients};
                ++$list->{total}{servers};
            }
            $list->{total_max}{clients} = $list->{total}{clients} if $list->{total_max}{clients} < $list->{total}{clients};
            $list->{total_max}{servers} = $list->{total}{servers} if $list->{total_max}{servers} < $list->{total}{servers};

            file_rewrite($config{list_pub}, JSON->new->encode($list));
            printlog "writed[$config{list_pub}] list size=", scalar @{$list->{list}} if $config{debug};

            $list->{list} = $list_full;
            file_rewrite($config{list_full}, JSON->new->encode($list));
            printlog "writed[$config{list_full}] list size=", scalar @{$list->{list}} if $config{debug};

        }
    };
    return [200, ["Content-type", "application/json"], [JSON->new->encode({})]], $after;
}

sub request_cgi {
    my ($p, $after) = request(@_);
    shift @$p;
    printu join "\n", map { join ': ', @$_ } shift @$p;
    printu "\n\n";
    printu join '', map { join '', @$_ } @$p;
    if (fork) {
        unless ($config{debug}) {
            close STDOUT;
            close STDERR;
        }
    } else {
        $after->() if ref $after ~~ 'CODE';
    }
}
request_cgi() unless caller;
