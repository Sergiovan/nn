use std::ops::Deref;
use std::ops::DerefMut;
use std::ops::Index;
use std::ops::IndexMut;

use std::marker::PhantomData;

#[derive(Eq, Debug)]
pub struct IndexedVecIndex<T>(u32, PhantomData<T>);

// Traits manually implemented to avoid T requirements, which are pointless
impl<T> Clone for IndexedVecIndex<T> {
	fn clone(&self) -> Self {
		*self
	}
}

impl<T> Copy for IndexedVecIndex<T> { }

impl<T> PartialEq for IndexedVecIndex<T> {
	fn eq(&self, other: &Self) -> bool {
		self.0 == other.0
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

impl<T> ToString for IndexedVecIndex<T> {
	fn to_string(&self) -> String {
		self.0.to_string()
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

#[macro_export]
macro_rules! ivec {
	($($x:tt)*) => {
		IndexedVec::from(vec![$($x)*])
	}
}

pub use ivec;