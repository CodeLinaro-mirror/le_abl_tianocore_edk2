%define debug_package %{nil}

Summary: Android Bootloader
Name: abl
Version: 1.0
Release: 1%{?dist}
Source0: %{name}-%{version}.tar.gz
License: BSD-2-Clause-Patent
Group: Development/Tools

BuildRequires: gcc clang libuuid-devel

%description
Add ABL SPEC for building it natively in AARCH64 environment

%prep
%setup -qn %{name}

%build
make BUILD_NATIVE_AARCH64=true

%install
cp %{_topdir}/abl.elf ${RPM_BUILD_ROOT}/unsigned_native_abl.elf

%files
/unsigned_native_abl.elf
