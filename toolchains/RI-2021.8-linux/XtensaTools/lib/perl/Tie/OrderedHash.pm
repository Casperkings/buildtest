# Copyright (c) 2018 Cadence Design Systems Inc. All rights reserved.
require 5.005;
package Tie::OrderedHash;
use strict;
use integer;
our $VERSION = '1.0';
sub TIEHASH {
  my $c = shift;
  my $s = [];
  $s->[0] = {};		
  $s->[1] = 0;		
  $s->[2] = $s;		
  $s->[3] = $s;		
  $s->[4] = $s;		
  bless $s, $c;
  return $s;
}
sub FETCH {
  my ($s, $k) = @_;
  my $e = $s->[0]{$k};
  return defined($e) ? $e->[1] : undef;
}
sub STORE {
  my ($s, $k, $v) = @_;
  my $h = $s->[0];
  my $e = $h->{$k};
  if (defined($e)) {
      $e->[1] = $v;
  } else {
      my $last = $s->[3];
      $h->{$k} = $e = [$k, $v, $s, $last];	
      $last->[2] = $e;				
      $s->[3] = $e;
      $s->[1]++;
  }
}
sub DELETE {
  my ($s, $k) = @_;
  my $e = $s->[0]{$k};
  return undef unless defined($e);
  $e->[2][3] = $e->[3];		
  $e->[3][2] = $e->[2];		
  $s->[1]--;
  return $e->[1];
}
sub EXISTS {
  my ($s, $k) = @_;
  return exists $s->[0]{$k};
}
sub FIRSTKEY {
  my ($s) = @_;
  $s->[4] = $s;
  &NEXTKEY;
}
sub NEXTKEY {
  my ($s) = @_;
  my $e = $s->[4] = $s->[4][2];		
  return ($e eq $s ? undef : $e->[0]);
}
sub CLEAR {
  my ($s) = @_;
  @$s = ({}, 0, $s, $s, $s);
}
sub SCALAR {
  my ($s) = @_;
  return $s->[1];	
}
sub new { TIEHASH(@_) }
sub Length { &SCALAR; }
1;
__END__
=head1 NAME
Tie::OrderedHash - quick and simple ordered associative arrays for Perl
=head1 SYNOPSIS
    use Tie::OrderedHash;
    tie HASHVARIABLE, 'Tie::OrderedHash';
=head1 DESCRIPTION
This Perl module implements Perl hashes that preserve the order in which the
hash elements were added.  The order is not affected when values
corresponding to existing keys are changed.
=item Reorder
This method reorders hash keys.  Any keys not in the list retain their
relative order and are left at the end of the reordered keys.
(NOT IMPLEMENTED)
=item Length
Returns the number of hash elements.
=back
=head1 EXAMPLE
    use Tie::OrderedHash;
    $t = tie(%myhash, 'Tie::IxHash');
    %myhash = (first => 1, second => 2, third => 3);
    $myhash{fourth} = 4;
    @keys = keys %myhash;
    @values = values %myhash;
    print("y") if exists $myhash{third};
    $len = $t->Length;     
=head1 AUTHOR
Copyright (c) 2018 Cadence Design Systems Inc. All rights reserved.
This program is free software; you can redistribute it and/or
modify it under the same terms as Perl itself.
=head1 VERSION
Version 1.0
=head1 SEE ALSO
perltie(1)
