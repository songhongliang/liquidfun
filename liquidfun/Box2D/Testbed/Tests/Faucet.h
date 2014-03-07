/*
* Copyright (c) 2014 Google, Inc.
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/
#ifndef FAUCET_H
#define FAUCET_H

// Faucet test creates a container from boxes and continually spawning
// particles with finite lifetimes that pour into the box.
class Faucet : public Test
{
public:
	// Construct the world.
	Faucet() :
		m_emitRemainder(0.0f),
		m_particleColorOffset(0.0f),
		m_emitRate(120.0f)
	{
		// Configure particle system parameters.
		m_particleSystem->SetRadius(0.035f);
		m_particleSystem->SetMaxParticleCount(k_maxParticleCount);
		m_particleSystem->SetDestructionByAge(true);

		b2Body* ground = NULL;
		{
			b2BodyDef bd;
			ground = m_world->CreateBody(&bd);
		}

		// Create the container / trough style sink.
		{
			b2PolygonShape shape;
			const float32 height = k_containerHeight + k_containerThickness;
			shape.SetAsBox(k_containerWidth - k_containerThickness,
						   k_containerThickness, b2Vec2(0.0f, 0.0f), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(k_containerThickness, height,
						   b2Vec2(-k_containerWidth, k_containerHeight), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(k_containerThickness, height,
						   b2Vec2(k_containerWidth, k_containerHeight), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Create ground under the container to catch overflow.
		{
			b2PolygonShape shape;
			shape.SetAsBox(k_containerWidth * 5.0f, k_containerThickness,
						   b2Vec2(0.0f, k_containerThickness * -2.0f), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Create the faucet spout.
		{
			b2PolygonShape shape;
			const float32 particleDiameter =
				m_particleSystem->GetRadius() * 2.0f;
			const float32 faucetLength = k_faucetLength * particleDiameter;
			// Dimensions of the faucet in world units.
			const float32 length = faucetLength * k_spoutLength;
			const float32 width = k_containerWidth * k_faucetWidth *
				k_spoutWidth;
			// Height from the bottom of the container.
			const float32 height = (k_containerHeight * k_faucetHeight) +
				(length * 0.5f);

			shape.SetAsBox(particleDiameter, length,
						   b2Vec2(-width, height), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(particleDiameter, length,
						   b2Vec2(width, height), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(width - particleDiameter, particleDiameter,
						   b2Vec2(0.0f, height + length -
								  particleDiameter), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Don't restart the test when changing particle types.
		TestMain::SetRestartOnParticleParameterChange(false);
		// Limit the set of particle types.
		TestMain::SetParticleParameters(k_paramDef, k_paramDefCount);
	}

	// Run a simulation step.
	void Step(Settings* settings)
	{
		const float32 dt = 1.0f / settings->hz;
		Test::Step(settings);
		m_particleColorOffset += dt;
		// Keep m_particleColorOffset in the range 0.0f..k_ParticleColorsCount.
		if (m_particleColorOffset >= (float32)k_ParticleColorsCount)
		{
			m_particleColorOffset -= (float32)k_ParticleColorsCount;
		}

		const float32 particles = (m_emitRate * dt) + m_emitRemainder;
		const int32 particlesToCreate = (int32)particles;
		const float32 faucetLength =
			m_particleSystem->GetRadius() * 2.0f * k_faucetLength;
		b2ParticleDef particleDef;

		// Propagate the currently selected particle flags.
		particleDef.flags = TestMain::GetParticleParameterValue();

		// If this is a color mixing particle, add some color.
		b2ParticleColor color(255, 255, 255, 255);
		if (particleDef.flags & b2_colorMixingParticle)
		{
			// Each second, select a different color.
			color =  k_ParticleColors[(int32)m_particleColorOffset %
									  k_ParticleColorsCount];
		}

		for (int32 i = 0; i < particlesToCreate; ++i)
		{
			// Emit particles from above the container.
			particleDef.position.Set(
				Random() * (k_containerWidth * k_faucetWidth),
				k_containerHeight * k_faucetHeight +
				(faucetLength * Random()));
			// Create a particle.
			const int32 index = m_particleSystem->CreateParticle(particleDef);
			// Set the particle's lifetime.
			m_particleSystem->SetParticleLifetime(
				index, Random() *
						(k_particleLifetimeMax - k_particleLifetimeMin) +
						k_particleLifetimeMin);
			// Set the particle's color.
			m_particleSystem->GetColorBuffer()[index] = color;
		}
		m_emitRemainder = (float32)(particles - particlesToCreate);

		static const char* k_keys[] = {
			"Keys: (w) water, (q) powder",
			"      (t) tensile, (v) viscous",
			"      (c) color mixing, (s) static pressure",
			"      (+) increase flow, (-) decrease flow",
		};
		for (uint32 i = 0; i < B2_ARRAY_SIZE(k_keys); ++i)
		{
			m_debugDraw.DrawString(5, m_textLine, k_keys[i]);
			m_textLine += DRAW_STRING_NEW_LINE;
		}
	}

	// Allows you to set particle flags on devices with keyboards
	void Keyboard(unsigned char key)
	{
		uint32 parameter = 0;
		switch (key)
		{
		case 'w':
			parameter = b2_waterParticle;
			break;
		case 'q':
			parameter = b2_powderParticle;
			break;
		case 't':
			parameter = b2_tensileParticle;
			break;
		case 'v':
			parameter = b2_viscousParticle;
			break;
		case 'c':
			parameter = b2_colorMixingParticle;
			break;
		case 's':
			parameter = b2_staticPressureParticle;
			break;
		case '+':
			m_emitRate *= k_emitRateChangeFactor;
			m_emitRate = b2Max(m_emitRate, k_emitRateMin);
			break;
		case '-':
			m_emitRate *= 1.0f / k_emitRateChangeFactor;
			m_emitRate = b2Min(m_emitRate, k_emitRateMax);
			break;
		default:
			// Nothing.
			return;
		}
		TestMain::SetParticleParameterValue(parameter);
	}

	float32 GetDefaultViewZoom() const
	{
		return 0.1f;
	}

	// Create the faucet test.
	static Test* Create()
	{
		return new Faucet;
	}

	// Generate a random number between -0.5f to 0.5f.
	static float32 Random()
	{
		return ((float32)rand() / (float32)RAND_MAX) - 0.5f;
	}

private:
	// Fractional particles to emit in the next Step().
	float32 m_emitRemainder;
	// Used to cycle through particle colors.
	float32 m_particleColorOffset;
	// Number of particles emitted per second.
	float32 m_emitRate;

private:
	// Minimum lifetime of particles in seconds.
	static const float32 k_particleLifetimeMin;
	// Maximum lifetime of particles in seconds.
	static const float32 k_particleLifetimeMax;
	// Height of the container.
	static const float32 k_containerHeight;
	// Width of the container.
	static const float32 k_containerWidth;
	// Thickness of the container's walls and bottom.
	static const float32 k_containerThickness;
	// Width of the faucet relative to the container width.
	static const float32 k_faucetWidth;
	// Height of the faucet relative to the base as a fraction of the
	// container height.
	static const float32 k_faucetHeight;
	// Length of the faucet as a fraction of the particle diameter.
	static const float32 k_faucetLength;
	// Spout height as a fraction of the faucet length.  This should be
	// greater than 1.0f).
	static const float32 k_spoutLength;
	// Spout width as a fraction of the *faucet* width.  This should be greater
	// than 1.0f).
	static const float32 k_spoutWidth;
	// Maximum number of particles in the system.
	static const int32 k_maxParticleCount;
	// Factor that is used to increase / decrease the emit rate.
	// This should be greater than 1.0f.
	static const float32 k_emitRateChangeFactor;
	// Minimum emit rate of the faucet in particles per second.
	static const float32 k_emitRateMin;
	// Maximum emit rate of the faucet in particles per second.
	static const float32 k_emitRateMax;

	// Selection of particle types for this test.
	static const ParticleParameter::Value k_paramValues[];
	static const ParticleParameter::Definition k_paramDef[];
	static const uint32 k_paramDefCount;
};

const float32 Faucet::k_particleLifetimeMin = 30.0f;
const float32 Faucet::k_particleLifetimeMax = 50.0f;
const float32 Faucet::k_containerHeight = 0.2f;
const float32 Faucet::k_containerWidth = 1.0f;
const float32 Faucet::k_containerThickness = 0.05f;
const float32 Faucet::k_faucetWidth = 0.1f;
const float32 Faucet::k_faucetHeight = 15.0f;
const float32 Faucet::k_faucetLength = 2.0f;
const float32 Faucet::k_spoutWidth = 1.1f;
const float32 Faucet::k_spoutLength = 2.0f;
const int32 Faucet::k_maxParticleCount = 1000;
const float32 Faucet::k_emitRateChangeFactor = 1.05f;
const float32 Faucet::k_emitRateMin = 1.0f;
const float32 Faucet::k_emitRateMax = 240.0f;

const ParticleParameter::Value Faucet::k_paramValues[] =
{
	{ b2_waterParticle, ParticleParameter::k_DefaultOptions, "water" },
	{ b2_waterParticle, ParticleParameter::k_DefaultOptions |
				ParticleParameter::OptionStrictContacts, "water (strict)" },
	{ b2_viscousParticle, ParticleParameter::k_DefaultOptions, "viscous" },
	{ b2_powderParticle, ParticleParameter::k_DefaultOptions, "powder" },
	{ b2_tensileParticle, ParticleParameter::k_DefaultOptions, "tensile" },
	{ b2_colorMixingParticle, ParticleParameter::k_DefaultOptions,
		"color mixing" },
	{ b2_staticPressureParticle, ParticleParameter::k_DefaultOptions,
		"static pressure" },
};

const ParticleParameter::Definition Faucet::k_paramDef[] =
{
	{ Faucet::k_paramValues, B2_ARRAY_SIZE(Faucet::k_paramValues) },
};

const uint32 Faucet::k_paramDefCount = B2_ARRAY_SIZE(Faucet::k_paramDef);

#endif  // FAUCET_H
