/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
// Input to player control mapping

#include <C4Include.h>
#include <C4PlayerControl.h>

#ifndef BIG_C4INCLUDE
#include <C4LangStringTable.h>
#include <C4Player.h>
#include <C4PlayerList.h>
#include <C4Control.h>
#include <C4Game.h>
#include <C4Log.h>
#include <C4GraphicsResource.h>
#include <C4MouseControl.h>
#endif

/* C4PlayerControlDef */

void C4PlayerControlDef::CompileFunc(StdCompiler *pComp)
	{
	if (!pComp->Name("ControlDef")) { pComp->NameEnd(); pComp->excNotFound("ControlDef"); }
	pComp->Value(mkNamingAdapt(mkParAdapt(sIdentifier, StdCompiler::RCT_Idtf), "Identifier", "None"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIName, StdCompiler::RCT_All), "GUIName", "undefined"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIDesc, StdCompiler::RCT_All), "GUIDesc", ""));
	pComp->Value(mkNamingAdapt(fGlobal, "Global", false));
	pComp->Value(mkNamingAdapt(fIsHoldKey, "Hold", false));
	pComp->Value(mkNamingAdapt(iRepeatDelay, "RepeatDelay", 0));
	pComp->Value(mkNamingAdapt(iInitialRepeatDelay, "InitialRepeatDelay", 0));
	pComp->Value(mkNamingAdapt(fDefaultDisabled, "DefaultDisabled", false));
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(idControlExtraData), "ExtraData", C4ID_None));
	const StdEnumEntry<Actions> ActionNames[] = {
		{ "None",        CDA_None        },
		{ "Script",      CDA_Script      },
		{ "Menu",        CDA_Menu        },
		{ "MenuOK",      CDA_MenuOK      },
		{ "MenuCancel",  CDA_MenuCancel  },
		{ "MenuLeft",    CDA_MenuLeft    },
		{ "MenuUp",      CDA_MenuUp      },
		{ "MenuRight",   CDA_MenuRight   },
		{ "MenuDown",    CDA_MenuDown    },
		{ NULL, CDA_None } };
	pComp->Value(mkNamingAdapt(mkEnumAdapt<Actions, int32_t>(eAction, ActionNames), "Action", CDA_Script));
	pComp->NameEnd();
	}

bool C4PlayerControlDef::operator ==(const C4PlayerControlDef &cmp) const
	{
	return sIdentifier == cmp.sIdentifier
	    && sGUIName == cmp.sGUIName
	    && sGUIDesc == cmp.sGUIDesc
	    && fGlobal == cmp.fGlobal
	    && fIsHoldKey == cmp.fIsHoldKey
	    && iRepeatDelay == cmp.iRepeatDelay
	    && iInitialRepeatDelay == cmp.iInitialRepeatDelay
	    && fDefaultDisabled == cmp.fDefaultDisabled
	    && idControlExtraData == cmp.idControlExtraData
	    && eAction == cmp.eAction;
	}


/* C4PlayerControlDefs */

void C4PlayerControlDefs::UpdateInternalCons()
	{
	InternalCons.CON_MenuSelect   = GetControlIndexByIdentifier("MenuSelect");
	InternalCons.CON_MenuEnter    = GetControlIndexByIdentifier("MenuEnter");
	InternalCons.CON_MenuEnterAll = GetControlIndexByIdentifier("MenuEnterAll");
	InternalCons.CON_MenuClose    = GetControlIndexByIdentifier("MenuClose");
	}

void C4PlayerControlDefs::Clear()
	{
	Defs.clear();
	UpdateInternalCons();
	}

void C4PlayerControlDefs::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Defs, StdCompiler::SEP_NONE), "ControlDefs", DefVecImpl()));
	if (pComp->isCompiler()) UpdateInternalCons();
	}

void C4PlayerControlDefs::MergeFrom(const C4PlayerControlDefs &Src)
	{
	// copy all defs from source file; overwrite defs of same name if found
	for (DefVecImpl::const_iterator i = Src.Defs.begin(); i != Src.Defs.end(); ++i)
		{
		const C4PlayerControlDef &SrcDef = *i;
		// overwrite if def of same name existed
		int32_t iPrevIdx = GetControlIndexByIdentifier(SrcDef.GetIdentifier());
		if (iPrevIdx != CON_None)
			{
			Defs[iPrevIdx] = SrcDef;
			}
		else
			{
			// new def: Append a copy
			Defs.push_back(SrcDef);
			}
		}
	UpdateInternalCons();
	}

const C4PlayerControlDef *C4PlayerControlDefs::GetControlByIndex(int32_t idx) const
	{
	// safe index
	if (idx<0 || idx>=int32_t(Defs.size())) return NULL;
	return &(Defs[idx]);
	}

int32_t C4PlayerControlDefs::GetControlIndexByIdentifier(const char *szIdentifier) const
	{
	for (DefVecImpl::const_iterator i = Defs.begin(); i != Defs.end(); ++i)
		if (SEqual((*i).GetIdentifier(), szIdentifier))
			return i-Defs.begin();
	return CON_None;
	}

void C4PlayerControlDefs::FinalInit()
	{
	// Assume all defs have been loaded
	// Register scritp constants
	for (DefVecImpl::const_iterator i = Defs.begin(); i != Defs.end(); ++i)
		{
		const char *szIdtf  = (*i).GetIdentifier();
		if (szIdtf && *szIdtf && !SEqual(szIdtf, "None"))
			{
			::ScriptEngine.RegisterGlobalConstant(FormatString("CON_%s", szIdtf).getData(), C4VInt(i-Defs.begin()));
			}
		}
	}


/* C4PlayerControlAssignment */

void C4PlayerControlAssignment::KeyComboItem::CompileFunc(StdCompiler *pComp)
	{
	// if key is compiled, also store as a string into KeyName for later resolving
	if (pComp->isCompiler())
		{
		sKeyName.Clear();
		pComp->Value(mkParAdapt(Key, &sKeyName));
		if (!sKeyName)
			{
			// key was not assigned during compilation - this means it's a regular key (or undefined)
			// store this as the name
			sKeyName.Copy(Key.ToString(false, false));
			}
		}
	else
		{
		// decompiler: Just write the stored key name; regardless of whether it's a key, undefined or a reference
		pComp->Value(mkParAdapt(sKeyName, StdCompiler::RCT_Idtf));
		}
	}

void C4PlayerControlAssignment::CompileFunc(StdCompiler *pComp)
	{
	if (!pComp->Name("Assignment")) { pComp->NameEnd(); pComp->excNotFound("Assignment"); }
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(KeyCombo), "Key", KeyComboVec()));
	pComp->Value(mkNamingAdapt(fComboIsSequence, "ComboIsSequence", false));
	pComp->Value(mkNamingAdapt(mkParAdapt(sControlName, StdCompiler::RCT_Idtf), "Control", "None"));
	pComp->Value(mkNamingAdapt(iPriority, "Priority", 0));
	const StdBitfieldEntry<int32_t> TriggerModeNames[] = {
		{ "Default",      CTM_Default  },
		{ "Hold",         CTM_Hold     },
		{ "Release",      CTM_Release  },
		{ "AlwaysUnhandled", CTM_AlwaysUnhandled  },
		{ NULL, 0 } };
	pComp->Value(mkNamingAdapt(mkBitfieldAdapt< int32_t>(iTriggerMode, TriggerModeNames), "TriggerMode", CTM_Default));
	pComp->NameEnd();
	// newly loaded structures are not resolved
	if (pComp->isCompiler()) fRefsResolved = false;
	}

bool C4PlayerControlAssignment::ResolveRefs(C4PlayerControlAssignmentSet *pParentSet, C4PlayerControlDefs *pControlDefs)
	{
	// avoid circular chains
	static C4PlayerControlAssignment *pCircularDetect = NULL;
	if (!pCircularDetect) pCircularDetect = this; else if (pCircularDetect == this)
		{
		LogFatal(FormatString("Circular reference chain detected in player control assignments of set %s in assignment for key %s!", pParentSet->GetName(), GetControlName()).getData());
		return false;
		}
	// resolve control name
	iControl = pControlDefs->GetControlIndexByIdentifier(sControlName.getData());
	// resolve keys
	KeyComboVec NewCombo;
	for (KeyComboVec::iterator i = KeyCombo.begin(); i != KeyCombo.end(); ++i)
		{
		KeyComboItem &rKeyComboItem = *i;
		if (rKeyComboItem.Key == KEY_Default && rKeyComboItem.sKeyName.getLength())
			{
			// this is a key reference
			// it may be preceded by CON_ to avoid ambigous keus
			const char *szKeyName = rKeyComboItem.sKeyName.getData();
			if (SEqual2(szKeyName, "CON_")) szKeyName +=4;
			// - find it
			C4PlayerControlAssignment *pRefAssignment = pParentSet->GetAssignmentByControlName(szKeyName);
			if (pRefAssignment)
				{
				// resolve itself if necessary
				if (!pRefAssignment->IsRefsResolved()) if (!pRefAssignment->ResolveRefs(pParentSet, pControlDefs)) return false;
				// insert all keys of that combo into own combo
				NewCombo.insert(NewCombo.end(), pRefAssignment->KeyCombo.begin(), pRefAssignment->KeyCombo.end());
				}
			else
				{
				// undefined reference? Not fatal, but inform user
				LogF("WARNING: Control %s of set %s contains reference to unassigned control %s.", GetControlName(), pParentSet->GetName(), rKeyComboItem.sKeyName.getData());
				NewCombo.clear();
				}
			}
		else
			{
			NewCombo.push_back(rKeyComboItem);
			}
		}
	KeyCombo = NewCombo;
	// the trigger key is always last of the chain
	if (KeyCombo.size()) TriggerKey = KeyCombo.back().Key; else TriggerKey = C4KeyCodeEx();
	// done
	fRefsResolved = true;
	if (pCircularDetect == this) pCircularDetect = NULL;
	return true;
	}

bool C4PlayerControlAssignment::IsComboMatched(const C4PlayerControlRecentKeyList &DownKeys, const C4PlayerControlRecentKeyList &RecentKeys) const
	{
	assert(HasCombo());
	// check if combo is currently fulfilled (assuming TriggerKey is already matched)
	if (fComboIsSequence)
		{
		DWORD tKeyLast = timeGetTime();
		// combo is a sequence: The last keys of RecentKeys must match the sequence
		// the last ComboKey is the TriggerKey, which is omitted because it has already been matched and is not to be found in RecentKeys yet
		C4PlayerControlRecentKeyList::const_reverse_iterator ri = RecentKeys.rbegin();
		for (KeyComboVec::const_reverse_iterator i = KeyCombo.rbegin()+1; i!=KeyCombo.rend(); ++i,++ri)
			{
			// no more keys pressed but combo didn't end? -> no combo match
			if (ri == RecentKeys.rend()) return false;
			const C4PlayerControlRecentKey &rk = *ri;
			// user waited for too long?
			DWORD tKeyRecent = rk.tTime;
			if (tKeyLast - tKeyRecent > C4PlayerControl::MaxSequenceKeyDelay) return false;
			// key doesn't match?
			const KeyComboItem &k = *i;
			if (!(rk.matched_key == k.Key)) return false;
			// key OK
			}
		}
	else
		{
		// combo requires keys to be down simultanuously: check that all keys of the combo are in the down-list
		for (KeyComboVec::const_iterator i = KeyCombo.begin(); i!=KeyCombo.end(); ++i)
			{
			const KeyComboItem &k = *i;
			bool fFound = false;
			for (C4PlayerControlRecentKeyList::const_iterator di = DownKeys.begin(); di!=DownKeys.end(); ++di)
				{
				const C4PlayerControlRecentKey &dk = *di;
				if (dk.matched_key == k.Key) { fFound = true; break; }
				}
			if (!fFound) return false;
			}
		}
	// combo OK!
	return true;
	}

bool C4PlayerControlAssignment::operator ==(const C4PlayerControlAssignment &cmp) const
	{
	// doesn't compare resolved TriggerKey/iControl
	return KeyCombo == cmp.KeyCombo
	    && sControlName == cmp.sControlName
	    && iTriggerMode == cmp.iTriggerMode
	    && iPriority == cmp.iPriority;
	}


/* C4PlayerControlAssignmentSet */

void C4PlayerControlAssignmentSet::CompileFunc(StdCompiler *pComp)
	{
	if (!pComp->Name("ControlSet")) { pComp->NameEnd(); pComp->excNotFound("ControlSet"); }
	pComp->Value(mkNamingAdapt(mkParAdapt(sName, StdCompiler::RCT_All), "Name", "None")); // can't do RCT_Idtf because of wildcards
	pComp->Value(mkNamingAdapt(has_keyboard, "Keyboard", true));
	pComp->Value(mkNamingAdapt(has_mouse, "Mouse", true));
	pComp->Value(mkNamingAdapt(has_gamepad, "Gamepad", false));
	pComp->Value(mkSTLContainerAdapt(Assignments, StdCompiler::SEP_NONE));
	pComp->NameEnd();
	}

void C4PlayerControlAssignmentSet::MergeFrom(const C4PlayerControlAssignmentSet &Src, bool fLowPrio)
	{
	// take over all assignments defined in Src
	for (C4PlayerControlAssignmentVec::const_iterator i = Src.Assignments.begin(); i != Src.Assignments.end(); ++i)
		{
		const C4PlayerControlAssignment &SrcAssignment = *i;
		// overwrite if def of same name existed if it's not low priority anyway?
		// not so easy. Keys may be assigned to multiple controls and we may need to overwrite one or more of them...
		C4PlayerControlAssignment *pPrevAssignment = GetAssignmentByControlName(SrcAssignment.GetControlName());
		if (pPrevAssignment)
			{
			if (!fLowPrio) *pPrevAssignment = SrcAssignment;
			}
		else
			{
			// new def: Append a copy
			Assignments.push_back(SrcAssignment);
			}
		}
	}

bool C4PlayerControlAssignmentSet::ResolveRefs(C4PlayerControlDefs *pDefs)
	{
	// resolve in order; ignore already resolved because they might have been resolved by cross reference
	for (C4PlayerControlAssignmentVec::iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		if (!(*i).IsRefsResolved())
			if (!(*i).ResolveRefs(this, pDefs))
				return false;
	// now sort assignments by priority
	std::sort(Assignments.begin(), Assignments.end());
	return true;
	}

C4PlayerControlAssignment *C4PlayerControlAssignmentSet::GetAssignmentByControlName(const char *szControlName)
{
	for (C4PlayerControlAssignmentVec::iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		if (SEqual((*i).GetControlName(), szControlName))
			// We don't like release keys... (2do)
			if (!((*i).GetTriggerMode() & C4PlayerControlAssignment::CTM_Release))
				return &*i;
	return NULL;
}

C4PlayerControlAssignment *C4PlayerControlAssignmentSet::GetAssignmentByControl(int control)
{
	// TODO: Might want to stuff this into a vector indexed by control for faster lookup
	for (C4PlayerControlAssignmentVec::iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		if ((*i).GetControl() == control)
			// We don't like release keys... (2do)
			if (!((*i).GetTriggerMode() & C4PlayerControlAssignment::CTM_Release))
				return &*i;
	return NULL;
}

bool C4PlayerControlAssignmentSet::operator ==(const C4PlayerControlAssignmentSet &cmp) const
	{
	return Assignments == cmp.Assignments
		&& sName == cmp.sName;
	}

void C4PlayerControlAssignmentSet::GetAssignmentsByKey(const C4PlayerControlDefs &rDefs, const C4KeyCodeEx &key, bool fHoldKeysOnly, C4PlayerControlAssignmentPVec *pOutVec, const C4PlayerControlRecentKeyList &DownKeys, const C4PlayerControlRecentKeyList &RecentKeys) const
	{
	assert(pOutVec);
	// primary match by TriggerKey
	for (C4PlayerControlAssignmentVec::const_iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		{
		const C4PlayerControlAssignment &rAssignment = *i;
		if (!(rAssignment.GetTriggerKey() == key)) continue;
		// check linked control def
		const C4PlayerControlDef *pCtrl = rDefs.GetControlByIndex(rAssignment.GetControl());
		if (!pCtrl) continue;
		// only want hold keys?
		if (fHoldKeysOnly)
			{
			// a hold/release-trigger key is not a real hold key, even if the underlying control is
			if (!pCtrl->IsHoldKey() || (rAssignment.GetTriggerMode() & (C4PlayerControlAssignment::CTM_Hold | C4PlayerControlAssignment::CTM_Release))) continue;
			}
		else if (rAssignment.HasCombo())
			{
			// hold-only events match the trigger key only (i.e., Release-events are generated as soon as the trigger key goes up)
			// other events must match either the sequence or the down-key-combination
			if (!rAssignment.IsComboMatched(DownKeys, RecentKeys)) continue;
			}
		// we got  match! Store it
		pOutVec->push_back(&rAssignment);
		}
	}

void C4PlayerControlAssignmentSet::GetTriggerKeys(const C4PlayerControlDefs &rDefs, C4KeyCodeExVec *pRegularKeys, C4KeyCodeExVec *pHoldKeys) const
	{
	// put all trigger keys of keyset into output vectors
	// first all hold keys
	for (C4PlayerControlAssignmentVec::const_iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		{
		const C4PlayerControlAssignment &rAssignment = *i;
		const C4PlayerControlDef *pDef = rDefs.GetControlByIndex(rAssignment.GetControl());
		if (pDef && pDef->IsHoldKey())
			{
			const C4KeyCodeEx &rKey = rAssignment.GetTriggerKey();
			if (std::find(pHoldKeys->begin(), pHoldKeys->end(), rKey) == pHoldKeys->end()) pHoldKeys->push_back(rKey);
			}
		}
	// then all regular keys that aren't in the hold keys list yet
	for (C4PlayerControlAssignmentVec::const_iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		{
		const C4PlayerControlAssignment &rAssignment = *i;
		const C4PlayerControlDef *pDef = rDefs.GetControlByIndex(rAssignment.GetControl());
		if (pDef && !pDef->IsHoldKey())
			{
			const C4KeyCodeEx &rKey = rAssignment.GetTriggerKey();
			if (std::find(pHoldKeys->begin(), pHoldKeys->end(), rKey) == pHoldKeys->end())
				if (std::find(pRegularKeys->begin(), pRegularKeys->end(), rKey) == pRegularKeys->end())
					pRegularKeys->push_back(rKey);
			}
		}
	}

C4Facet C4PlayerControlAssignmentSet::GetPicture() const
{
	// get image to be drawn to represent this control set
	// picture per set not implemented yet. So just default to out standard images
	if (HasGamepad()) return ::GraphicsResource.fctGamepad.GetPhase(GetGamepadIndex());
	if (HasKeyboard()) return ::GraphicsResource.fctKeyboard.GetPhase(0 /* todo*/);
	if (HasMouse()) return ::GraphicsResource.fctMouse; // mouse only???
	return C4Facet();
}

bool C4PlayerControlAssignmentSet::IsMouseControlAssigned(int32_t mouseevent) const
{
	// TODO
	return true;
}


/* C4PlayerControlAssignmentSets */

void C4PlayerControlAssignmentSets::Clear()
	{
	Sets.clear();
	}

void C4PlayerControlAssignmentSets::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Sets, StdCompiler::SEP_NONE), "ControlSets", AssignmentSetList()));
	}

void C4PlayerControlAssignmentSets::MergeFrom(const C4PlayerControlAssignmentSets &Src, bool fLowPrio)
	{
	// take over all assignments in known sets and new sets defined in Src
	for (AssignmentSetList::const_iterator i = Src.Sets.begin(); i != Src.Sets.end(); ++i)
		{
		const C4PlayerControlAssignmentSet &SrcSet = *i;
		// overwrite if def of same name existed if it's not low priority anyway
		bool fIsWildcardSet = SrcSet.IsWildcardName();
		if (!fIsWildcardSet)
			{
			C4PlayerControlAssignmentSet *pPrevSet = GetSetByName(SrcSet.GetName());
			if (pPrevSet)
				{
				pPrevSet->MergeFrom(SrcSet, fLowPrio);
				}
			else
				{
				// new def: Append a copy
				Sets.push_back(SrcSet);
				}
			}
		else
			{
			// source is a wildcard: Merge with all matching sets
			for (AssignmentSetList::iterator j = Sets.begin(); j != Sets.end(); ++j)
				{
				C4PlayerControlAssignmentSet &DstSet = *j;
				if (WildcardMatch(SrcSet.GetName(), DstSet.GetName()))
					{
					DstSet.MergeFrom(SrcSet, fLowPrio);
					}
				}
			}
		}
	}

bool C4PlayerControlAssignmentSets::ResolveRefs(C4PlayerControlDefs *pDefs)
	{
	for (AssignmentSetList::iterator i = Sets.begin(); i != Sets.end(); ++i)
		if (!(*i).ResolveRefs(pDefs)) return false;
	return true;
	}

C4PlayerControlAssignmentSet *C4PlayerControlAssignmentSets::GetSetByName(const char *szName)
{
	for (AssignmentSetList::iterator i = Sets.begin(); i != Sets.end(); ++i)
		if (WildcardMatch(szName, (*i).GetName()))
			return &*i;
	return NULL;
}

C4PlayerControlAssignmentSet *C4PlayerControlAssignmentSets::GetDefaultSet()
{
	// default set is first defined control set
	if (Sets.empty()) return NULL; // nothing defined :(
	return &Sets.front();
}

int32_t C4PlayerControlAssignmentSets::GetSetIndex(const C4PlayerControlAssignmentSet *set) const
{
	// find set in list; return index
	int32_t index = 0;
	for (AssignmentSetList::const_iterator i = Sets.begin(); i != Sets.end(); ++i,++index)
		if (&*i == set)
			return index;
	return -1; // not found
}

C4PlayerControlAssignmentSet *C4PlayerControlAssignmentSets::GetSetByIndex(int32_t index)
{
	// bounds check
	if (index < 0 || index >= (int32_t)Sets.size()) return NULL;
	// return indexed set
	AssignmentSetList::iterator i = Sets.begin();
	while (index--) ++i;
	return &*i;
}


/* C4PlayerControlFile */

void C4PlayerControlFile::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(ControlDefs);
	pComp->Value(AssignmentSets);
	}

bool C4PlayerControlFile::Load(C4Group &hGroup, const char *szFilename, C4LangStringTable *pLang)
	{
	// clear previous
	Clear();
	// load and prepare file contents
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(szFilename, Buf)) return false;
	if (pLang) pLang->ReplaceStrings(Buf);
	// parse it!
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, Buf, szFilename)) return false;
	return true;
	}

bool C4PlayerControlFile::Save(C4Group &hGroup, const char *szFilename)
	{
	// decompile to buffer and save buffer to group
	StdStrBuf Buf;
	if (!DecompileToBuf_Log<StdCompilerINIWrite>(*this, &Buf, szFilename)) return false;
	hGroup.Add(szFilename, Buf, false, true);
	return true;
	}

void C4PlayerControlFile::Clear()
	{
	ControlDefs.Clear();
	AssignmentSets.Clear();
	}


/* C4PlayerControl */

void C4PlayerControl::CSync::ControlDownState::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(DownState);
	pComp->Seperator();
	pComp->Value(iDownFrame);
	pComp->Seperator();
	pComp->Value(fDownByUser);
	}

bool C4PlayerControl::CSync::ControlDownState::operator ==(const ControlDownState &cmp) const
	{
	return DownState == cmp.DownState && iDownFrame == cmp.iDownFrame && fDownByUser == cmp.fDownByUser;
	}

const C4PlayerControl::CSync::ControlDownState *C4PlayerControl::CSync::GetControlDownState(int32_t iControl) const
	{
	// safe access
	if (iControl < 0 || iControl >= int32_t(ControlDownStates.size())) return NULL;
	return &ControlDownStates[iControl];
	}

int32_t C4PlayerControl::CSync::GetControlDisabled(int32_t iControl) const
	{
	// safe access
	if (iControl < 0 || iControl >= int32_t(ControlDisableStates.size())) return 0;
	return ControlDisableStates[iControl];
	}

void C4PlayerControl::CSync::SetControlDownState(int32_t iControl, const C4KeyEventData &rDownState, int32_t iDownFrame, bool fDownByUser)
	{
	// update state
	if (iControl < 0) return;
	if (iControl >= int32_t(ControlDownStates.size())) ControlDownStates.resize(iControl+1);
	ControlDownState &rState = ControlDownStates[iControl];
	rState.DownState = rDownState;
	rState.iDownFrame = iDownFrame;
	rState.fDownByUser = fDownByUser;
	}

void C4PlayerControl::CSync::SetControlDisabled(int32_t iControl, int32_t iVal)
	{
	// disable control
	if (iControl < 0) return;
	if (iControl >= int32_t(ControlDisableStates.size())) ControlDisableStates.resize(iControl+1);
	ControlDisableStates[iControl] = iVal;
	// if a control is disabled, its down-state is reset silently
	const ControlDownState *pDownState = GetControlDownState(iControl);
	if (pDownState && pDownState->IsDown())
		{
		C4KeyEventData KeyDownState = pDownState->DownState;
		KeyDownState.iStrength = 0;
		SetControlDownState(iControl, KeyDownState, 0, false);
		}
	}

void C4PlayerControl::CSync::Clear()
	{
	ControlDownStates.clear();
	ControlDisableStates.clear();
	}

void C4PlayerControl::CSync::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(ControlDownStates), "Down", DownStateVec()));
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(ControlDisableStates), "Disabled", DisableStateVec()));
	}

bool C4PlayerControl::CSync::operator ==(const CSync &cmp) const
	{
	return ControlDownStates == cmp.ControlDownStates
	    && ControlDisableStates == cmp.ControlDisableStates;
	}

void C4PlayerControl::CompileFunc(StdCompiler *pComp)
	{
	// compile sync values only
	pComp->Value(mkNamingAdapt(Sync, "PlayerControl", CSync()));
	}

bool C4PlayerControl::ProcessKeyEvent(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key, bool fUp, const C4KeyEventData &rKeyExtraData)
	{
	// collect all matching keys
	C4PlayerControlAssignmentPVec Matches;
	assert(pControlSet); // shouldn't get this callback for players without control set
	pControlSet->GetAssignmentsByKey(ControlDefs, matched_key, fUp, &Matches, DownKeys, RecentKeys);
	// process async controls
	C4ControlPlayerControl *pControlPacket = NULL;
	for (C4PlayerControlAssignmentPVec::const_iterator i = Matches.begin(); i != Matches.end(); ++i)
		{
		const C4PlayerControlAssignment *pAssignment = *i;
		assert(pAssignment);
		int32_t iControlIndex = pAssignment->GetControl();
		const C4PlayerControlDef *pControlDef = ControlDefs.GetControlByIndex(iControlIndex);
		if (pControlDef && pControlDef->IsValid() && (!fUp || pControlDef->IsHoldKey()))
			{
			if (pControlDef->IsAsync() && !pControlPacket)
				{
				if (ExecuteControl(iControlIndex, fUp, rKeyExtraData, pAssignment->GetTriggerMode(), pressed_key.IsRepeated()))
					return true;
				}
			else
				{
				// sync control
				// ignore key repeats, because we do our own key repeat for sync controls
				if (pressed_key.IsRepeated()) return false;
				// sync control has higher priority - no more async execution then
				// build a control packet and add control data instead. even for async controls later in chain, as they may be blocked by a sync handler
				if (!pControlPacket) pControlPacket = new C4ControlPlayerControl(iPlr, fUp, rKeyExtraData);
				pControlPacket->AddControl(iControlIndex, pAssignment->GetTriggerMode());
				}
			}
		}
	// push sync control to input
	if (pControlPacket)
		{
		Game.Input.Add(CID_PlrControl2, pControlPacket);
		// assume processed (although we can't really know that yet)
		return true;
		}
	return false;
	}

bool C4PlayerControl::ProcessKeyDown(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key)
{
	// add key to local "down" list if it's not already in there
	// except for some mouse events for which a down state does not make sense
	C4PlayerControlRecentKey RKey(pressed_key,matched_key,timeGetTime());
	if (!Key_IsMouse(pressed_key.Key) || Inside<uint8_t>(Key_GetMouseEvent(pressed_key.Key), KEY_MOUSE_Button1, KEY_MOUSE_ButtonMax))
	{
		if (std::find(DownKeys.begin(), DownKeys.end(), pressed_key) == DownKeys.end()) DownKeys.push_back(RKey);
	}
	// process!
	bool fResult = ProcessKeyEvent(pressed_key, matched_key, false, Game.KeyboardInput.GetLastKeyExtraData());
	// add to recent list unless repeated
	if (!pressed_key.IsRepeated()) RecentKeys.push_back(RKey);
	return fResult;
}

bool C4PlayerControl::ProcessKeyUp(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key)
{
	// remove key from "down" list
	// except for some mouse events for which a down state does not make sense
	if (!Key_IsMouse(pressed_key.Key) || Inside<uint8_t>(Key_GetMouseEvent(pressed_key.Key), KEY_MOUSE_Button1, KEY_MOUSE_ButtonMax))
	{
		C4PlayerControlRecentKeyList::iterator i = find(DownKeys.begin(), DownKeys.end(), pressed_key);
		if (i != DownKeys.end()) DownKeys.erase(i);
	}
	// process!
	return ProcessKeyEvent(pressed_key, matched_key, true, Game.KeyboardInput.GetLastKeyExtraData());
}

void C4PlayerControl::ExecuteControlPacket(const class C4ControlPlayerControl *pCtrl)
	{
	// callback from control queue. Execute controls in packet until one of them gets processed
	// assume async packets always as not processed to ensure sync safety (usually, sync commands should better not ovberride async commands anyway)
	for (C4ControlPlayerControl::ControlItemVec::const_iterator i = pCtrl->GetControlItems().begin(); i != pCtrl->GetControlItems().end(); ++i)
		{
		const C4ControlPlayerControl::ControlItem &rItem = *i;
		const C4PlayerControlDef *pCtrlDef = ControlDefs.GetControlByIndex(rItem.iControl);
		if (pCtrlDef)
			{
			if (ExecuteControl(rItem.iControl, pCtrl->IsReleaseControl(), pCtrl->GetExtraData(), rItem.iTriggerMode, false))
				if (pCtrlDef->IsSync())
					break;
			}
		}
	}

bool C4PlayerControl::ExecuteControl(int32_t iControl, bool fUp, const C4KeyEventData &rKeyExtraData, int32_t iTriggerMode, bool fRepeated)
	{
	// execute single control. return if handled
	const C4PlayerControlDef *pControlDef = ControlDefs.GetControlByIndex(iControl);
	if (!pControlDef || Sync.IsControlDisabled(iControl)) return false;
	C4PlayerControlDef::Actions eAction = pControlDef->GetAction();
	C4KeyEventData KeyExtraData(rKeyExtraData);
	const CSync::ControlDownState *pCtrlDownState = Sync.GetControlDownState(iControl);
	bool fWasDown = pCtrlDownState ? pCtrlDownState->IsDown() : false;
	// global controls only in global context
	if (IsGlobal() != pControlDef->IsGlobal()) return false;
	// hold-actions only work on script controls with the hold flag
	if (iTriggerMode & (C4PlayerControlAssignment::CTM_Hold | C4PlayerControlAssignment::CTM_Release))
		{
		if (eAction != C4PlayerControlDef::CDA_Script) return false;
		if (!pControlDef->IsHoldKey()) return false;
		if (fUp) return false; // hold triggers have no "up"-event
		// perform hold/release
		if (fWasDown)
			{
			// control is currently down: release?
			if (iTriggerMode & C4PlayerControlAssignment::CTM_Release)
				{
				KeyExtraData.iStrength = 0;
				Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, false);
				// now process as a regular "Up" event
				fUp = true;
				fRepeated = false;
				}
			else //if (iTriggerMode & C4PlayerControlAssignment::CTM_Hold) - must be true
				{
				// control is down but trigger key is pressed again: Refresh down state
				// (this will restart the KeyRepeat time)
				Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, false);
				// now process as a regular, repeated "down" event
				fRepeated = true;
				}
			}
		else
			{
			// control is currently up. Put into hold-down-state if this is a hold key
			if (iTriggerMode & C4PlayerControlAssignment::CTM_Hold)
				{
				Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, false);
				// now process as a regular "down" event
				fRepeated = false;
				}
			else
				{
				//. Ignore if it's only a release key
				return false;
				}
			}
		}
	else if (fUp)
		{
		// regular ControlUp: Only valid if that control was down
		if (!fWasDown) return false;
		Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, true);
		}
	else if (pControlDef->IsHoldKey())
		{
		// regular ControlDown on Hold Key: Set in down list
		Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, true);
		fRepeated = fWasDown;
		}
	// perform action for this control
	bool fHandled = ExecuteControlAction(iControl, eAction, pControlDef->GetExtraData(), fUp, KeyExtraData, fRepeated);
	// handled controls hide control display
	C4Player *pPlr;
	if (pPlr = ::Players.Get(iPlr)) if (pPlr->ShowStartup) pPlr->ShowStartup = false;
	// return if handled, unless control is defined as always unhandled
	return fHandled && !(iTriggerMode & C4PlayerControlAssignment::CTM_AlwaysUnhandled);
	}

bool C4PlayerControl::ExecuteControlAction(int32_t iControl, C4PlayerControlDef::Actions eAction, C4ID idControlExtraData, bool fUp, const C4KeyEventData &rKeyExtraData, bool fRepeated)
	{
	// get affected player
	C4Player *pPlr = NULL;
	if (iPlr > -1)
		{
		pPlr = ::Players.Get(iPlr);
		if (!pPlr) return false;
		}
	// exec action (on player)
	switch (eAction)
		{
		// scripted player control
		case C4PlayerControlDef::CDA_Script:
			return ExecuteControlScript(iControl, idControlExtraData, fUp, rKeyExtraData, fRepeated);

		// menu controls
		case C4PlayerControlDef::CDA_Menu: if (!pPlr || fUp) return false; if (pPlr->Menu.IsActive()) pPlr->Menu.Close(false); else pPlr->ActivateMenuMain(); return true; // toggle
		case C4PlayerControlDef::CDA_MenuOK:     if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuEnter,0); return true; // ok on item
		case C4PlayerControlDef::CDA_MenuCancel: if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuClose,0); return true; // close menu
		case C4PlayerControlDef::CDA_MenuLeft:   if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuLeft ,0); return true; // navigate
		case C4PlayerControlDef::CDA_MenuUp:     if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuUp   ,0); return true; // navigate
		case C4PlayerControlDef::CDA_MenuRight:  if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuRight,0); return true; // navigate
		case C4PlayerControlDef::CDA_MenuDown:   if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuDown ,0); return true; // navigate

		//unknown action
		default: return false;
		}
	}

bool C4PlayerControl::ExecuteControlScript(int32_t iControl, C4ID idControlExtraData, bool fUp, const C4KeyEventData &rKeyExtraData, bool fRepeated)
	{
	C4Player *pPlr = ::Players.Get(iPlr);
	if (pPlr)
		{
		// Not for eliminated (checked again in DirectCom, but make sure no control is generated for eliminated players!)
		if (pPlr->Eliminated) return false;
		// control count for statistics
		pPlr->CountControl(C4Player::PCID_DirectCom, iControl*2+fUp);
		}
	else if (iPlr > -1)
		{
		// player lost?
		return false;
		}
	// control down
	C4AulFunc *pFunc = ::ScriptEngine.GetFirstFunc(PSF_PlayerControl);
	if (!pFunc) return false;
	C4AulParSet Pars(C4VInt(iPlr), C4VInt(iControl), C4VID(idControlExtraData), C4VInt(rKeyExtraData.x), C4VInt(rKeyExtraData.y), C4VInt(rKeyExtraData.iStrength), C4VBool(fRepeated), C4VBool(fUp));
	return !!pFunc->Exec(NULL, &Pars);
	}


void C4PlayerControl::Execute()
{
	// sync execution: Do keyrepeat
	for (int32_t i=0; i<ControlDefs.GetCount(); ++i)
	{
		const CSync::ControlDownState *pControlDownState = Sync.GetControlDownState(i);
		if (pControlDownState && pControlDownState->IsDown())
		{
			const C4PlayerControlDef *pCtrlDef = ControlDefs.GetControlByIndex(i);
			assert(pCtrlDef);
			int32_t iCtrlRepeatDelay = pCtrlDef->GetRepeatDelay();
			if (iCtrlRepeatDelay)
			{
				int32_t iFrameDiff = Game.FrameCounter - pControlDownState->iDownFrame;
				int32_t iCtrlInitialRepeatDelay = pCtrlDef->GetInitialRepeatDelay();
				if (iFrameDiff && iFrameDiff >= iCtrlInitialRepeatDelay)
				{
					if (!((iFrameDiff-iCtrlInitialRepeatDelay) % iCtrlRepeatDelay))
					{
						// it's RepeatTime for this key!
						ExecuteControlAction(i, pCtrlDef->GetAction(), pCtrlDef->GetExtraData(), false, pControlDownState->DownState, true);
					}
				}
			}
		}
	}
	// cleanup old recent keys
	C4PlayerControlRecentKeyList::iterator irk;
	DWORD tNow = timeGetTime();
	for (irk = RecentKeys.begin(); irk != RecentKeys.end(); ++irk)
	{
		C4PlayerControlRecentKey &rk = *irk;
		if (rk.tTime + MaxRecentKeyLookback > tNow) break;
	}
	if (irk != RecentKeys.begin()) RecentKeys.erase(RecentKeys.begin(), irk);
}

C4PlayerControl::C4PlayerControl() : ControlDefs(Game.PlayerControlDefs), iPlr(-1), pControlSet(NULL)
{
}

void C4PlayerControl::Clear()
{
	iPlr = NO_OWNER;
	pControlSet = NULL;
	for (KeyBindingList::iterator i = KeyBindings.begin(); i != KeyBindings.end(); ++i) delete *i;
	KeyBindings.clear();
	RecentKeys.clear();
	DownKeys.clear();
	Sync.Clear();
}

void C4PlayerControl::RegisterKeyset(int32_t iPlr, C4PlayerControlAssignmentSet *pKeyset)
{
	// clear any previous settings
	Clear();
	// setup
	pControlSet = pKeyset;
	this->iPlr = iPlr;
	// register all keys into Game.KeyboardInput creating KeyBindings
	if (pControlSet)
	{
		C4KeyCodeExVec RegularKeys, HoldKeys;
		pControlSet->GetTriggerKeys(ControlDefs, &RegularKeys, &HoldKeys);
		int32_t idx=0;
		for (C4KeyCodeExVec::const_iterator i = RegularKeys.begin(); i != RegularKeys.end(); ++i) AddKeyBinding(*i, false, idx++);
		for (C4KeyCodeExVec::const_iterator i = HoldKeys.begin(); i != HoldKeys.end(); ++i) AddKeyBinding(*i, true, idx++);
	}
}

void C4PlayerControl::AddKeyBinding(const C4KeyCodeEx &key, bool fHoldKey, int32_t idx)
{
	KeyBindings.push_back(new C4KeyBinding(
				key, FormatString("PlrKey%02d", idx).getData(), KEYSCOPE_Control,
				new C4KeyCBExPassKey<C4PlayerControl, C4KeyCodeEx>(*this, key, &C4PlayerControl::ProcessKeyDown, fHoldKey ? &C4PlayerControl::ProcessKeyUp : NULL),
				C4CustomKey::PRIO_PlrControl));
}

bool C4PlayerControl::DoMouseInput(uint8_t mouse_id, int32_t mouseevent, float game_x, float game_y, float gui_x, float gui_y, bool is_ctrl_down, bool is_shift_down, bool is_alt_down)
{
	// convert moueevent to key code
	uint8_t mouseevent_code;
	C4KeyCodeEx mouseevent_keycode;
	bool is_down = false;
	switch (mouseevent)
	{
		case C4MC_Button_None: mouseevent_code = KEY_MOUSE_Move; break;
		case C4MC_Button_LeftDown: is_down = true; // nobreak
		case C4MC_Button_LeftUp: mouseevent_code = KEY_MOUSE_ButtonLeft; break;
		case C4MC_Button_RightDown: is_down = true; // nobreak
		case C4MC_Button_RightUp: mouseevent_code = KEY_MOUSE_ButtonLeft; break;
		case C4MC_Button_LeftDouble: mouseevent_code = KEY_MOUSE_ButtonLeftDouble; break;
		case C4MC_Button_RightDouble: mouseevent_code = KEY_MOUSE_ButtonRightDouble; break;
		case C4MC_Button_Wheel: mouseevent_code = KEY_MOUSE_ButtonMiddleDouble; break;
		case C4MC_Button_MiddleDown: is_down = true; // nobreak
		case C4MC_Button_MiddleUp: mouseevent_code = KEY_MOUSE_ButtonMiddle; break;
		default: assert(false); return false;
	}
	// compose keycode
	if (is_ctrl_down) mouseevent_keycode.dwShift |= KEYS_Control;
	if (is_shift_down) mouseevent_keycode.dwShift |= KEYS_Shift;
	if (is_alt_down) mouseevent_keycode.dwShift |= KEYS_Alt;
	mouseevent_keycode.Key = KEY_Mouse(mouse_id, mouseevent_code, false);
	// first, try processing it as GUI mouse event. if not assigned, process as Game mous event
	// TODO: May route this through Game.DoKeyboardInput instead - would allow assignment of mouse events in CustomConfig
	//  and would get rid of the Game.KeyboardInput.SetLastKeyExtraData-hack
	C4KeyEventData mouseevent_data;
	mouseevent_data.iStrength = 100; // TODO: May get pressure from tablet here
	mouseevent_data.x = uint32_t(gui_x);
	mouseevent_data.y = uint32_t(gui_y);
	Game.KeyboardInput.SetLastKeyExtraData(mouseevent_data); // ProcessKeyDown/Up queries it from there...
	bool result;
	if (is_down)
		result = ProcessKeyDown(mouseevent_keycode, mouseevent_keycode);
	else
		result = ProcessKeyUp(mouseevent_keycode, mouseevent_keycode);
	if (result)
	{
		// mouse event processed in GUI coordinates
		return true;
	}
	// try processing in Game coordinates instead
	mouseevent_data.x = uint32_t(game_x);
	mouseevent_data.y = uint32_t(game_y);
	mouseevent_keycode.Key |= KEY_MOUSE_GameMask;
	if (is_down)
		result = ProcessKeyDown(mouseevent_keycode, mouseevent_keycode);
	else
		result = ProcessKeyUp(mouseevent_keycode, mouseevent_keycode);
	return result;
}
