// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <iostream>
#include <vector>
#include "Phrase.h"
#include "TypeDef.h"
#include "WordsBitmap.h"
#include "Sentence.h"
#include "Phrase.h"
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"
#include "LanguageModel.h"
#include "ScoreComponentCollection.h"
#include "LexicalReordering.h"
#include "Input.h"

class SquareMatrix;
class StaticData;
class TranslationOption;
class WordsRange;
class WordDeletionTable;
class Hypothesis;

typedef std::vector<Hypothesis*> ArcList;

class Hypothesis
{
	friend std::ostream& operator<<(std::ostream&, const Hypothesis&);
private:
  // MD5 checksum for fast comparison
  unsigned char m_compSignature[16];

protected:

	const Hypothesis* m_prevHypo;
	Phrase					m_targetPhrase; //target phrase being created at the current decoding step
	WordsBitmap				m_sourceCompleted;
	//TODO: how to integrate this into confusion network framework; what if
	//it's a confusion network in the end???
	//how to make a general InputType without getting abstract warnings.
	InputType const&  m_sourceInput;
	WordsRange				m_currSourceWordsRange, m_currTargetWordsRange;
  bool							m_wordDeleted;
	float							m_totalScore;
	float							m_futureScore;
	ScoreComponentCollection2 m_scoreBreakdown;	
#ifdef N_BEST
	const Hypothesis 	*m_mainHypo;
	ArcList 					*m_arcList; //all arcs that end at the same lattice point as we do
#endif

//	 * \return whether none of the factors clash
//	 * \param phrase TODO ???
//	 */
	bool IsCompatible(const Phrase &phrase) const;
	
	void CalcFutureScore(const SquareMatrix &futureScore);
	//void CalcFutureScore(float futureScore[256][256]);
	void CalcLMScore(const LMList		&languageModels);
	void CalcDistortionScore();
	//TODO: add appropriate arguments to score calculator

  void CalcDeletionScore(const Sentence& sourceSentence, const WordsRange& sourceWordsRange, const WordDeletionTable& wordDeletionTable);

	void GenerateNGramCompareHash() const;
	mutable size_t _hash;
	mutable bool _hash_computed;

public:


	static unsigned int s_HypothesesCreated; // Statistics: how many hypotheses were created in total
	static unsigned int s_numNodes; // Statistics: how many hypotheses were created in total
	int m_id;
	
	Hypothesis(InputType const& source);
		// used for initial seeding of trans process
	Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt);
		// create next
	~Hypothesis();
	
	/***
	 * return the subclass of Hypothesis most appropriate to the given translation option
	 */
	static Hypothesis* Create(const Hypothesis &prevHypo, const TranslationOption &transOpt);
	/***
	 * return the subclass of Hypothesis most appropriate to the given target phrase
	 */

	static Hypothesis* Create(const WordsBitmap &initialCoverage);
	/***
	 * return the subclass of Hypothesis most appropriate to the given target phrase
	 */
	static Hypothesis* Create(InputType const& source);


	/***
	 * return the subclass of Hypothesis most appropriate to the given translation option
	 */
	Hypothesis* CreateNext(const TranslationOption &transOpt) const;
	/***
	 * if any factors aren't set in our target phrase but are present in transOpt, copy them over
	 * (unless the factors that we do have fail to match the corresponding ones in transOpt,
	 *  in which case presumably there's a programmer's error)
	 * 
	 * return NULL if we aren't compatible with the given option
	 */

	void PrintHypothesis(  const InputType &source, float weightDistortion, float weightWordPenalty) const;

	const Phrase &GetTargetPhrase() const
	{
		return m_targetPhrase;
	}

 // void PrintLMScores(const LMList &lmListInitial, const LMList	&lmListEnd) const;
	inline const WordsRange &GetCurrSourceWordsRange() const
	{
		return m_currSourceWordsRange;
	}
	
	inline const WordsRange &GetCurrTargetWordsRange() const
	{
		return m_currTargetWordsRange;
	}
	
	// subsequent translation should only translate this sub-phrase
	size_t GetCurrTargetLength() const
	{
		return m_currTargetWordsRange.GetWordsCount();
	}

	void ResetScore();

	void CalcScore(const StaticData& staticData, const SquareMatrix &futureScore);

	int GetId() const;


	const Hypothesis* GetPrevHypo() const;

	// same as for phrase
	inline size_t GetSize() const
	{
		return m_currTargetWordsRange.GetEndPos() + 1;
	}
	inline const Phrase &GetPhrase() const
	{
		return m_targetPhrase;
	}
	inline const InputType &GetSourcePhrase() const
	{
		return m_sourceInput;
	}

	// curr - pos is relative from CURRENT hypothesis's starting ind ex
  // (ie, start of sentence would be some negative number, which is
  // not allowed- USE WITH CAUTION)
	inline FactorArray &GetCurrFactorArray(size_t pos)
	{
		return m_targetPhrase.GetFactorArray(pos);
	}
	inline const FactorArray &GetCurrFactorArray(size_t pos) const
	{
		return m_targetPhrase.GetFactorArray(pos);
	}
	inline const Factor *GetCurrFactor(size_t pos, FactorType factorType) const
	{
		return m_targetPhrase.GetFactor(pos, factorType);
	}
	// recursive - pos is relative from start of sentence
	inline const FactorArray &GetFactorArray(size_t pos) const
	{
		const Hypothesis *hypo = this;
		while (pos < hypo->GetCurrTargetWordsRange().GetStartPos())
		{
			hypo = hypo->GetPrevHypo();
		}
		assert(hypo != NULL);
		return hypo->GetCurrFactorArray(pos - hypo->GetCurrTargetWordsRange().GetStartPos());
	}
	inline const Factor* GetFactor(size_t pos, FactorType factorType) const
	{
		return GetFactorArray(pos)[factorType];
	}

	/***
	 * \return The bitmap of source words we cover
	 */
	inline const WordsBitmap &GetWordsBitmap() const
	{
		return m_sourceCompleted;
	}

	inline size_t hash() const
	{
		if (_hash_computed) return _hash;
		GenerateNGramCompareHash();
		return _hash;
	}

  /***
   * requires that GenerateNGramCompareKey was previously run
   */
	int	FastNGramCompare(const Hypothesis &compare) const
	{
#if 0
		return memcmp(this->m_compSignature, compare.m_compSignature, sizeof(m_compSignature));
#else
		return NGramCompare(compare);
#endif
	}

	int	 NGramCompare(const Hypothesis &compare) const;

  /*** Generates a key in m_compSignature that can be used
   *   for an arbitrary ordering of hypotheses.
   *
   * With this key, hypotheses that have identical target
   * strings for the last n elements are considered equal.
   *
   * Currently not used since performance is approximately
   * identical to the regular comparison engine.  This may
   * be a better option with higher n-gram LMs.
   */
  void GenerateNGramCompareKey(size_t n);

	void ToStream(std::ostream& out) const
	{
		if (m_prevHypo != NULL)
		{
			m_prevHypo->ToStream(out);
		}
		out << GetPhrase();
	}

	TO_STRING;

#ifdef N_BEST
	inline void SetMainHypo(const Hypothesis *hypo)
	{
		m_mainHypo = hypo;
	}
	void AddArc(Hypothesis *loserHypo);
	inline void InitializeArcs()
	{
		if (!m_arcList) return;
		ArcList::iterator iter = m_arcList->begin();
		for (; iter != m_arcList->end() ; ++iter)
		{
			Hypothesis *arc = *iter;
			arc->SetMainHypo(this);
		}
	}
	inline const ArcList* GetArcList() const
	{
		return m_arcList;
	}
#endif
	const ScoreComponentCollection2& GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}
	float GetTotalScore() const { return m_totalScore; }
	float GetFutureScore() const { return m_futureScore; }
};


std::ostream& operator<<(std::ostream& out, const Hypothesis& hypothesis);