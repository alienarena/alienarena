import pytest
import ctypes
import struct


# Simulate the decal vertex processing logic in Python
# to test the security invariant: buffer reads never exceed declared length

class DecalRawVertex(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("z", ctypes.c_float),
        ("u", ctypes.c_float),
        ("v", ctypes.c_float),
    ]

DECAL_VERTEX_SIZE = ctypes.sizeof(DecalRawVertex)  # 20 bytes


def simulate_decal_copy(declared_capacity: int, vertex_data: bytes) -> bytes:
    """
    Simulates the memcpy behavior from r_decals.c:660
    
    Security invariant: the number of bytes copied must never exceed
    declared_capacity * DECAL_VERTEX_SIZE
    
    Returns the safely copied data (truncated if necessary) or raises
    ValueError if the input would cause an overflow.
    """
    max_allowed_bytes = declared_capacity * DECAL_VERTEX_SIZE
    
    # Enforce the invariant: reject or truncate oversized input
    if len(vertex_data) > max_allowed_bytes:
        raise ValueError(
            f"Buffer overflow prevented: input size {len(vertex_data)} "
            f"exceeds declared capacity {declared_capacity} vertices "
            f"({max_allowed_bytes} bytes)"
        )
    
    return vertex_data[:max_allowed_bytes]


def build_vertex_payload(num_vertices: int) -> bytes:
    """Build a raw vertex payload for the given number of vertices."""
    payload = b""
    for i in range(num_vertices):
        # Pack as 5 floats (x, y, z, u, v)
        payload += struct.pack("5f", float(i), float(i), float(i), 0.5, 0.5)
    return payload


@pytest.mark.parametrize("declared_capacity,vertex_count,should_overflow", [
    # Normal cases - should succeed
    (10, 10, False),
    (10, 5, False),
    (1, 1, False),
    (100, 50, False),
    
    # Overflow cases - 2x declared capacity
    (10, 20, True),
    (5, 10, True),
    (1, 2, True),
    (100, 200, True),
    
    # Overflow cases - 10x declared capacity
    (10, 100, True),
    (5, 50, True),
    (1, 10, True),
    (100, 1000, True),
    
    # Edge cases - exactly one over capacity
    (10, 11, True),
    (1, 2, True),
    (255, 256, True),
    
    # Attacker-controlled map data: large declared capacity but even larger actual data
    (1024, 2048, True),
    (4096, 40960, True),
    
    # Zero capacity edge case
    (0, 1, True),
    (0, 0, False),
    
    # Malicious map file: tiny declared capacity, huge vertex count
    (2, 65535, True),
    (1, 32768, True),
])
def test_buffer_reads_never_exceed_declared_length(
    declared_capacity: int,
    vertex_count: int,
    should_overflow: bool
):
    """
    Invariant: Buffer reads (memcpy) must never exceed the declared length.
    
    Simulates the vulnerability in r_decals.c:660 where memcpy copies vertex
    data using newidx*sizeof(decalrawvertex_t) as size. If newidx exceeds
    the allocated capacity of out->verts, a heap buffer overflow occurs.
    
    This test ensures that:
    1. Oversized inputs are rejected (not silently overflowing)
    2. Valid inputs within declared capacity are accepted
    3. The copy size never exceeds declared_capacity * sizeof(decalrawvertex_t)
    """
    vertex_data = build_vertex_payload(vertex_count)
    
    # Verify our payload is correctly sized
    expected_payload_size = vertex_count * DECAL_VERTEX_SIZE
    assert len(vertex_data) == expected_payload_size, (
        f"Payload construction error: expected {expected_payload_size} bytes, "
        f"got {len(vertex_data)} bytes"
    )
    
    max_allowed_bytes = declared_capacity * DECAL_VERTEX_SIZE
    
    if should_overflow:
        # Oversized input MUST be rejected to prevent heap buffer overflow
        with pytest.raises(ValueError, match="Buffer overflow prevented"):
            simulate_decal_copy(declared_capacity, vertex_data)
        
        # Additional assertion: verify the input truly exceeds capacity
        assert len(vertex_data) > max_allowed_bytes, (
            f"Test setup error: vertex_data ({len(vertex_data)} bytes) should "
            f"exceed max_allowed_bytes ({max_allowed_bytes} bytes)"
        )
    else:
        # Valid input should succeed without overflow
        result = simulate_decal_copy(declared_capacity, vertex_data)
        
        # CRITICAL INVARIANT: result must never exceed declared capacity
        assert len(result) <= max_allowed_bytes, (
            f"SECURITY VIOLATION: copied {len(result)} bytes but declared "
            f"capacity is only {max_allowed_bytes} bytes "
            f"({declared_capacity} vertices)"
        )
        
        # Verify the data integrity for valid copies
        assert result == vertex_data, (
            "Valid data should be copied without modification"
        )


@pytest.mark.parametrize("payload_bytes", [
    # Raw byte payloads simulating attacker-controlled map file data
    b"\xff" * (DECAL_VERTEX_SIZE * 100),   # 100 vertices of 0xFF
    b"\x00" * (DECAL_VERTEX_SIZE * 50),    # 50 null vertices
    b"\x41" * (DECAL_VERTEX_SIZE * 200),   # 200 'A' vertices
    b"\x90" * (DECAL_VERTEX_SIZE * 1000),  # NOP sled style payload
    b"\xde\xad\xbe\xef" * (DECAL_VERTEX_SIZE * 25),  # Pattern payload
])
def test_raw_oversized_payloads_are_rejected(payload_bytes):
    """
    Invariant: Raw oversized byte payloads from attacker-controlled map files
    must be rejected before any buffer copy operation.
    
    Tests that adversarial raw byte inputs exceeding a small declared capacity
    are always rejected, preventing heap buffer overflow.
    """
    # Simulate a small declared capacity (e.g., from a map file header)
    declared_capacity = 10
    max_allowed_bytes = declared_capacity * DECAL_VERTEX_SIZE
    
    # All payloads in this test are designed to exceed the declared capacity
    assert len(payload_bytes) > max_allowed_bytes, (
        f"Test setup error: payload ({len(payload_bytes)} bytes) must exceed "
        f"declared capacity ({max_allowed_bytes} bytes)"
    )
    
    # The oversized payload MUST be rejected
    with pytest.raises(ValueError, match="Buffer overflow prevented"):
        simulate_decal_copy(declared_capacity, payload_bytes)


def test_copy_size_invariant_holds_for_all_valid_inputs():
    """
    Invariant: For any valid input, the number of bytes copied must equal
    exactly vertex_count * sizeof(decalrawvertex_t) and must not exceed
    declared_capacity * sizeof(decalrawvertex_t).
    """
    for declared_capacity in [1, 5, 10, 50, 100]:
        for vertex_count in range(0, declared_capacity + 1):
            vertex_data = build_vertex_payload(vertex_count)
            result = simulate_decal_copy(declared_capacity, vertex_data)
            
            max_allowed = declared_capacity * DECAL_VERTEX_SIZE
            
            # Core invariant: never exceed declared capacity
            assert len(result) <= max_allowed, (
                f"SECURITY VIOLATION at capacity={declared_capacity}, "
                f"vertices={vertex_count}: "
                f"copied {len(result)} > allowed {max_allowed}"
            )
            
            # Correctness: exact copy for valid data
            assert len(result) == vertex_count * DECAL_VERTEX_SIZE