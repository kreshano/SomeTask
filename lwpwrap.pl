package lwp;

#
# Common LWP Wrapper
#

use strict;
use warnings;
use LWP;
use LWP::UserAgent;
use LWP::Simple;
use HTTP::Cookies;
use HTTP::Request::Common;
use URI::Escape;
use HTTP::Request;
use Data::Dumper;

my ($ua, $jar);
my ($host, $port, $username, $passwd, $realm);
my $ConfFile = 'conf/host';
my $cookie_file = 'tmp/3dpcookie';
$realm = "Internal accounts";

my $url;
my $debug = undef;

sub _debug_lwp
{
  print $ua->as_string;
}

sub new
{
    my ($class, $args) = @_;
    
    #Cookie file will be created if missing
    $jar = new HTTP::Cookies (file => $cookie_file, autosave =>1);
    
    ($host, $port, $username, $passwd) = readConf();
    $url = (($port == 80) ? "http://" : "https://").$host;

    $ua = new LWP::UserAgent ();
    #setting up lwp
    $ua->cookie_jar ($jar);
    $ua->credentials ($host.":".$port, $realm, $username, $passwd);

    #adding debuging heandler
    $ua->add_handler("request_send",  sub { shift->dump if( $debug ); $debug = undef; return } );
    $ua->add_handler("response_done", sub { shift->dump if( $debug ); $debug = undef; return } );
    my $this = { ua => $ua, jar => $jar };
    
    bless $this, $class;
    return ($this);
}

sub readConf()
{
    open my $info, $ConfFile or die "Could not open $ConfFile: $!";

    my @conf;
    while( my $line = <$info>) {
        chop($line);        
        push @conf,$line   if( length($line) and substr($line,0,1) ne '#' );
    }
    
    return @conf;
}

sub prepareUrl
{
    my ($this, $resource, $query) = @_;
    
    my $u = URI->new($url.$resource);
    $u->query_form( %$query );
    
    return $u;
}

sub login 
{
    my ($this, $url) = @_;
    my $req = HTTP::Request->new(GET => $url);
    $req->authorization_basic($username, $passwd);
    my $resp = $ua->request($req);
    die ("Failed to auth at $url with $username and $passwd") unless $resp->code == 200;
    print STDERR Dumper $jar;
}

sub getPayload
{
    my ($this, $url, $params, $isDebug) = @_;
    $debug = $isDebug;
    
    if (keys %{$params}) {
        if ($url =~ /\?/ && $url !~ /&$/) {
            $url .= "&";
        } elsif ($url !~ /\?/) {
            $url .= "?";
        }
        foreach (keys (%{$params})) {
            $url .= uri_escape($_)."=".uri_escape ($$params{$_})."&";
        }
        chop $url;
    }
    
    #print STDERR Dumper $jar;
    my $resp = $ua->get ($url);
    #print STDERR Dumper $jar;
    return ($resp->code, $resp->content, $resp);
}

sub getConfHost
{
    return $host;
}

sub postPayload 
{
    my ($this, $url, $params, $isDebug) = @_;
    $debug = $isDebug;
    
    #print STDERR Dumper $jar;
    my $resp = $ua->post ($url, %$params);
    #print STDERR Dumper $jar;
    return ($resp->code, $resp->content, $resp);
}

sub putPayload
{
    
    my ($this, $url, $params, $isDebug) = @_;
    $debug = $isDebug;
    
    my $request  = HTTP::Request::Common::PUT($url);
    
    $request->header( 'Content-Type' => $$params{'Content-Type'} );
    $request->add_content( $$params{'Content'} );
   
    my $resp = $ua->request($request);;
    
    return ($resp->code, $resp->content, $resp);
}

sub deletePayload
{
    my ($this, $url, $params, $isDebug) = @_;
    $debug = $isDebug;
    
    my $request  = HTTP::Request::Common::DELETE($url);
    my $resp = $ua->request($request);
    
    return ($resp->code, $resp->content, $resp);    
}

1;


package common;

#
# Common Interface to use that Wrapper
#

require lwp;

our $LWP_THIS = new lwp;
our $THIS;
my $ur;

sub getResource
{
    my ($res, $query, $isDirect, $isDebug) = @_;
    my ($code, $content, $resp);
    
    if( !(defined $isDirect) ) {
        $url = $LWP_THIS->prepareUrl($res, $query);
    } else {
        $url = $res;
    }
    
    ($code, $content, $resp) = $LWP_THIS->getPayload ( $url, undef, $isDebug );
    verifyResponseCode($resp);
    
    return ($resp); 
}

sub getHost
{
    my $host = $LWP_THIS->getConfHost();
}

sub postResource
{
    my ($res, $type, $xmlSource, $isDirect, $isString, $isDebug) = @_;
    my ($code, $content, $resp);
    my $postData;
    
    if( defined $isString ){
        $postData = $xmlSource;
    }else {
        $postData = _readXml($xmlSource);    
    }

    my %param = (
              'Content-Type' => $type,
               Content  =>  $postData,
               );

    if( !(defined $isDirect) ) {
        $url = $LWP_THIS->prepareUrl($res,{});
    } else {
        $url = $res;
    }          
    
    ($code, $content, $resp) = $LWP_THIS->postPayload ( $url, \%param, $isDebug);
    verifyResponseCode($resp);
    
    return ($resp);
}

sub putResource
{
    my ($res, $type, $xmlFile, $isDirect, $isString, $isDebug) = @_;
    my ($code, $content, $resp);
    my $postData;

    if( defined $isString ){
        $postData = $xmlFile;
    }else {
        $postData = _readXml($xmlFile);    
    }
    
    my %param = (
              'Content-Type' => $type,
              'Expect' => '',
               Content  =>  $postData
               );

    if( !(defined $isDirect) ) {
        $url = $LWP_THIS->prepareUrl($res,{});
    } else {
        $url = $res;
    } 
    
    ($code, $content, $resp) = $LWP_THIS->putPayload ( $url, \%param, $isDebug);
    verifyResponseCode($resp);
    
    return ($resp);
}

sub deleteResource
{
    my ($res, $isDirect, $isDebug) = @_;
    my ($code, $content, $resp);
    
    if( !(defined $isDirect) ) {
        $url = $LWP_THIS->prepareUrl($res,{});
    } else {
        $url = $res;
    }
    
    ($code, $content, $resp) = $LWP_THIS->deletePayload ( $url, undef, $isDebug );
    verifyResponseCode($resp);
    
    return ($resp);
}

1;
