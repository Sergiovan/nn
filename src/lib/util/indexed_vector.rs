use std::fmt::Display;
use std::ops::Deref;
use std::ops::DerefMut;
use std::ops::Index;
use std::ops::IndexMut;

use std::marker::PhantomData;

#[derive(Eq)]
pub struct IndexedVecIndex<T>(pub u32, PhantomData<T>);

// Traits manually implemented to avoid T requirements, which are pointless
impl<T> Clone for IndexedVecIndex<T> {
	fn clone(&self) -> Self {
		*self
	}
}

impl<T> Copy for IndexedVecIndex<T> {}

impl<T> PartialEq for IndexedVecIndex<T> {
	fn eq(&self, other: &Self) -> bool {
		self.0 == other.0
	}
}

impl<T> PartialEq<u32> for IndexedVecIndex<T> {
	fn eq(&self, other: &u32) -> bool {
		self.0 == *other
	}
}

impl<T> PartialEq<usize> for IndexedVecIndex<T> {
	fn eq(&self, other: &usize) -> bool {
		self.0 as usize == *other
	}
}

impl<T> PartialOrd for IndexedVecIndex<T> {
	fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
		self.0.partial_cmp(&other.0)
	}
}

impl<T> PartialOrd<u32> for IndexedVecIndex<T> {
	fn partial_cmp(&self, other: &u32) -> Option<std::cmp::Ordering> {
		self.0.partial_cmp(other)
	}
}

impl<T> PartialOrd<usize> for IndexedVecIndex<T> {
	fn partial_cmp(&self, other: &usize) -> Option<std::cmp::Ordering> {
		(self.0 as usize).partial_cmp(other)
	}
}

impl<T> From<u32> for IndexedVecIndex<T> {
	fn from(id: u32) -> Self {
		IndexedVecIndex(id, PhantomData)
	}
}

impl<T> From<usize> for IndexedVecIndex<T> {
	fn from(id: usize) -> Self {
		assert!(id <= u32::MAX as usize);
		IndexedVecIndex(id as u32, PhantomData)
	}
}

impl<T> Display for IndexedVecIndex<T> {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		self.0.fmt(f)
	}
}

impl<T> core::fmt::Debug for IndexedVecIndex<T> {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		f.debug_tuple("Index").field(&self.0).finish()
	}
}

pub trait IndexedVector {
	type Index: Copy;
}

#[derive(Debug)]
pub struct IndexedVec<T>(Vec<T>);

impl<T> IndexedVector for IndexedVec<T> {
	type Index = IndexedVecIndex<T>;
}

impl<T> IndexedVec<T> {
	pub fn valid_index(&self, idx: IndexedVecIndex<T>) -> bool {
		(idx.0 as usize) < self.len()
	}
}

impl<T> Deref for IndexedVec<T> {
	type Target = Vec<T>;

	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl<T> DerefMut for IndexedVec<T> {
	fn deref_mut(&mut self) -> &mut Self::Target {
		&mut self.0
	}
}

impl<T> Index<IndexedVecIndex<T>> for IndexedVec<T> {
	type Output = T;

	fn index(&self, index: IndexedVecIndex<T>) -> &Self::Output {
		&self.0[index.0 as usize]
	}
}

impl<T> IndexMut<IndexedVecIndex<T>> for IndexedVec<T> {
	fn index_mut(&mut self, index: IndexedVecIndex<T>) -> &mut Self::Output {
		&mut self.0[index.0 as usize]
	}
}

impl<T> From<Vec<T>> for IndexedVec<T> {
	fn from(value: Vec<T>) -> Self {
		IndexedVec(value)
	}
}

impl<T> IntoIterator for IndexedVec<T> {
	type Item = <Vec<T> as IntoIterator>::Item;

	type IntoIter = <Vec<T> as IntoIterator>::IntoIter;

	fn into_iter(self) -> Self::IntoIter {
		self.0.into_iter()
	}
}

#[macro_export]
macro_rules! ivec {
	($($x:tt)*) => {
		IndexedVec::from(vec![$($x)*])
	}
}

pub use ivec;
