/*
    Copyright 2011 Thibaut Paumard

    This file is part of Gyoto.

    Gyoto is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Gyoto is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gyoto.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GyotoTorus.h"
#include "GyotoStandardAstrobj.h"
#include "GyotoUtils.h"
#include "GyotoBlackBodySpectrum.h"
#include "GyotoPowerLawSpectrum.h"
#include "GyotoMetric.h"
#include "GyotoProperty.h"
#include "GyotoFactoryMessenger.h"

#include <float.h>
#include <cmath>
#include <iostream>
#include <cstdlib>

using namespace Gyoto;
using namespace Gyoto::Astrobj;
using namespace std;

GYOTO_PROPERTY_START(Torus,
		     "Geometrical Torus in circular rotation.")
GYOTO_PROPERTY_SPECTRUM(Torus, Spectrum, spectrum,
			"Emission law.")
GYOTO_PROPERTY_SPECTRUM(Torus, Opacity, opacity,
			"Absorption law.")
GYOTO_PROPERTY_DOUBLE(Torus, SmallRadius, smallRadius,
		      "Minor radius, radius of a meridian circle.")
GYOTO_PROPERTY_DOUBLE(Torus, LargeRadius, largeRadius,
		      "Major radius, distance from centre of tube to centre of torus. ")
GYOTO_PROPERTY_END(Torus, Standard::properties)

Torus::Torus() : Standard("Torus"),
	  c_(3.5)
{
  critical_value_ = 0.25; // 0.5*0.5
  safety_value_ = 0.3;
  spectrum_ = new Spectrum::BlackBody(1000000.);
  opacity_ = new Spectrum::PowerLaw(0., 1.);
}

Torus::Torus(const Torus& o)
  : Standard(o),
    c_(o.c_),
    spectrum_(o.spectrum_()?o.spectrum_->clone():NULL),
    opacity_(o.opacity_()?o.opacity_->clone():NULL)
{}

Torus::~Torus() {}

Torus* Torus::clone() const { return new Torus(*this); }

double Torus::largeRadius() const { return c_; }
double Torus::largeRadius(string unit) const {
  return Units::FromGeometrical(largeRadius(), unit, gg_);
}
double Torus::smallRadius() const { return sqrt(critical_value_); }
double Torus::smallRadius(string unit) const {
  return Units::FromGeometrical(smallRadius(), unit, gg_);
}

void Torus::largeRadius(double c) { c_ = c; }
void Torus::largeRadius(double c, string unit) {
  largeRadius(Units::ToGeometrical(c, unit, gg_));
}

void Torus::smallRadius(double a) {
  critical_value_ = a*a;
  safety_value_ = critical_value_ * 1.1;
}
void Torus::smallRadius(double c, string unit) {
  smallRadius(Units::ToGeometrical(c, unit, gg_));
}

SmartPointer<Spectrum::Generic> Torus::spectrum() const { return spectrum_; }
void Torus::spectrum(SmartPointer<Spectrum::Generic> sp) {spectrum_=sp;}

SmartPointer<Spectrum::Generic> Torus::opacity() const { return opacity_; }
void Torus::opacity(SmartPointer<Spectrum::Generic> sp) {opacity_=sp;}

double Torus::rMax() {
  if (rmax_==DBL_MAX) {
    rmax_ = 3.*(c_+sqrt(critical_value_));
  }
  return rmax_ ;
}

double Torus::emission(double nu_em, double dsem, double *, double *) const {
  if (flag_radtransf_) return (*spectrum_)(nu_em, (*opacity_)(nu_em), dsem);
  return (*spectrum_)(nu_em);
}

double Torus::transmission(double nuem, double dsem, double*) const {
  if (!flag_radtransf_) return 0.;
  double opac = (*opacity_)(nuem);
  if (debug())
    cerr << "DEBUG: Torus::transmission(nuem="<<nuem<<", dsem="<<dsem<<"), "
	 << "opacity=" << opac << "\n";
  if (!opac) return 1.;
  return exp(-opac*dsem);
}

double Torus::integrateEmission(double nu1, double nu2, double dsem,
			       double *, double *) const {
  if (flag_radtransf_)
    return spectrum_->integrate(nu1, nu2, opacity_(), dsem);
  return spectrum_->integrate(nu1, nu2);
}

double Torus::operator()(double const pos[4]) {
  double drproj, h;
  switch (gg_->coordKind()) {
  case GYOTO_COORDKIND_SPHERICAL:
    drproj = pos[1]*sin(pos[2])-c_;
    h = pos[1]*cos(pos[2]);
    break;
  case GYOTO_COORDKIND_CARTESIAN:
    h = pos[3];
    drproj = sqrt(pos[1]*pos[1]+pos[2]*pos[2])-c_;
    break;
  default:
    throwError("Torus::distance(): unknown coordinate system kind");
    h=0.,drproj=0.;
  }
  return drproj*drproj + h*h;
}

double Torus::deltaMax(double * coord) {
  double d2 = (*this)(coord);
  if (d2<critical_value_) d2 = critical_value_;
  return 0.1 * sqrt(d2);
}

void Torus::getVelocity(double const pos[4], double vel[4]) {
  double pos2[4] = {pos[0]};
  switch (gg_ -> coordKind()) {
  case GYOTO_COORDKIND_CARTESIAN:
    pos2[1] = pos[1];
    pos2[2] = pos[2];
    pos2[3] = 0.;
    break;
  case GYOTO_COORDKIND_SPHERICAL:
    pos2[1] = pos[1] * sin(pos[2]);
    pos2[2] = M_PI*0.5;
    pos2[3] = pos[3];
    break;
  default:
    throwError("Torus::getVelocity(): unknown coordkind");
  }
  gg_ -> circularVelocity(pos2, vel);
}
