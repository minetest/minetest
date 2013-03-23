#!/usr/bin/perl

=info
install:
 cpan JSON JSON::XS
 touch list_full list
 chmod a+rw list_full list

freebsd:
 www/fcgiwrap www/nginx

rc.conf.local:
nginx_enable="YES"
fcgiwrap_enable="YES"
fcgiwrap_user="www"

nginx:

        location / {
            index  index.html;
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


=cut

use strict;
no strict qw(refs);
use warnings "NONFATAL" => "all";
no warnings qw(uninitialized);
use utf8;
use Socket;
use Time::HiRes qw(time sleep);
use IO::Socket::INET;
use JSON;
use Net::Ping;
our $root_path;
($ENV{'SCRIPT_FILENAME'} || $0) =~ m|^(.+)[/\\].+?$|;    #v0w
$root_path = $1 . '/' if $1;
$root_path =~ s|\\|/|g;

our %config = (
    #debug        => 1,
    list_full    => $root_path . 'list_full',
    list_pub     => $root_path . 'list',
    time_purge   => 86400 * 30,
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
    warn "json error [$@] on [", ${ref $_[0] ? $_[0] : \$_[0]}, "]" if $@;
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

sub name_to_ip_noc($) {
    my ($name) = @_;
    unless ($name =~ /^\d+\.\d+\.\d+\.\d+$/) {
        local $_ = (gethostbyname($name))[4];
        return ($name, 1) unless length($_) == 4;
        $name = inet_ntoa($_);
    }
    return $name;
}

sub float {
    return ($_[0] < 8 and $_[0] - int($_[0]))
      ? sprintf('%.' . ($_[0] < 1 ? 3 : ($_[0] < 3 ? 2 : 1)) . 'f', $_[0])
      : int($_[0]);

}

sub mineping ($$) {
    my ($addr, $port) = @_;
    warn "mineping($addr, $port)" if $config{debug};
    my $data;
    my $time = time;
    eval {
        my $socket = IO::Socket::INET->new(
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
    warn "recvd: ", length $data, " [$time]" if $config{debug};
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
        if (%$param) {
            s/^false$// for values %$param;
            $param->{ip} = $r->{REMOTE_ADDR};
            for (@{$config{blacklist}}) {
                return if $param->{ip} ~~ $_;
            }
            $param->{address} ||= $param->{ip};
            if ($config{source_check} and name_to_ip_noc($param->{address}) ne $param->{ip} and !($param->{ip} ~~ $config{trusted})) {
                warn("bad address [$param->{address}] ne [$param->{ip}]") if $config{debug};
                return;
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
                        warn "strange ping ip [$ip] != [$param->{ip}]" if $config{debug};
                        return if $config{source_check} and !($param->{ip} ~~ $config{trusted});
                    }
                    $param->{ping} = $duration if $pingret;
                    warn " PING t=$config{ping_timeout}, $param->{address}:$param->{port} = ( $pingret, $duration, $ip )" if $config{debug};
                }
            }
            my $list = read_json($config{list_full}) || {};
            warn "readed[$config{list_full}] list size=", scalar @{$list->{list}};
            my $listk = {map { $_->{key} => $_ } @{$list->{list}}};
            my $old = $listk->{$param->{key}};
            $param->{time} = $old->{time} if $param->{off};
            $param->{time} ||= int time;
            $param->{start} = $param->{action} ~~ 'start' ? $param->{time} : $old->{start} || $param->{time};
            delete $param->{start} if $param->{off};
            $param->{first} ||= $old->{first} || $old->{time} || $param->{time};
            $param->{clients_top} = $old->{clients_top} if $old->{clients_top} > $param->{clients};
            $param->{clients_top} ||= $param->{clients} || 0;
            delete $param->{action};
            $listk->{$param->{key}} = $param;
            $list->{list} = [grep { $_->{time} > time - $config{time_purge} } values %$listk];
            file_rewrite($config{list_full}, JSON->new->encode($list));
            warn "writed[$config{list_full}] list size=", scalar @{$list->{list}};
            $list->{list} = [
                sort { $b->{clients} <=> $a->{clients} || $a->{start} <=> $b->{start} }
                  grep { $_->{time} > time - $config{time_alive} and !$_->{off} and (!$config{ping} or !$config{pingable} or $_->{ping}) }
                  @{$list->{list}}
            ];
            file_rewrite($config{list_pub}, JSON->new->encode($list));
            warn "writed[$config{list_pub}] list size=", scalar @{$list->{list}};
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
